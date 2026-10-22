# SPDX-License-Identifier: LicenseRef-EnvNode-Use-and-Redistribution-1.0
"""Fixed-template PDF pilot: consume the actual MkDocs-generated article."""
from pathlib import Path
import hashlib, json, re, os
from urllib.parse import urljoin, urlparse, unquote
from bs4 import BeautifulSoup
from jinja2 import Environment, FileSystemLoader, select_autoescape
from weasyprint import HTML, CSS, default_url_fetcher
from pypdf import PdfReader


def render(config, document):
    root=Path(config.config_file_path).resolve().parent
    site=Path(config['site_dir']).resolve()
    page=site/Path(document['source']).with_suffix('')/'index.html'
    source=root/'docs/site'/document['source']
    import yaml
    meta=yaml.safe_load(source.read_text().split('---',2)[1])['pdf']
    required=['title','subtitle','hardware_revision','document_version','document_date','status','source_document_date']
    for k in required:
        if not isinstance(meta.get(k),str) or not meta[k].strip():raise ValueError(f'Missing PDF metadata: {k}')
    soup=BeautifulSoup(page.read_text(), 'html.parser')
    article=soup.select_one('article.md-content__inner')
    if article is None:raise ValueError('MkDocs article missing')
    for elem in article.select('.headerlink,.md-button'):elem.decompose()
    # Resolve URLs exactly as the generated page does, and forbid network fetches.
    for img in article.select('img[src]'):
        url=urljoin(page.as_uri(),img['src']); path=Path(unquote(urlparse(url).path))
        if not path.is_file() or not path.is_relative_to(site):raise ValueError(f'Invalid PDF image: {url}')
        img['src']=path.as_uri()
    for a in article.select('a[href]'):
        href=a['href']
        if not href.startswith(('#','http:','https:','mailto:')):
            # Keep navigation usable in the standalone PDF, not as local file URLs.
            path=urlparse(urljoin(page.as_uri(),href)).path
            relative=Path(unquote(path)).relative_to(site)
            a['href']=config['site_url'].rstrip('/')+'/'+relative.as_posix()
    # Share the introductory source text with the generated cover, avoiding
    # an almost empty duplicate title page in the print edition.
    intro=BeautifulSoup('<div></div>','html.parser').div
    while article.contents and getattr(article.contents[0], 'name', None) != 'h2':
        child=article.contents[0].extract()
        if getattr(child,'name',None) != 'h1':intro.append(child)
    template=root/'docs/pdf/template.html';style=root/'docs/pdf/print.css'
    webstyle=root/'docs/site/assets/stylesheets/envnode.css'
    colors=dict(re.findall(r'(--envnode-[a-z-]+):\s*(#[0-9A-Fa-f]{6})',webstyle.read_text()))
    if len(colors)<9:raise ValueError('Shared EnvNode palette incomplete')
    font_dir=Path(os.environ.get('ENVNODE_PDF_FONT_DIR','/usr/share/fonts/truetype/dejavu')).resolve()
    fonts=[font_dir/'DejaVuSans.ttf',font_dir/'DejaVuSans-Bold.ttf',font_dir/'DejaVuSansMono.ttf']
    if not all(p.is_file() for p in fonts):raise FileNotFoundError('Set ENVNODE_PDF_FONT_DIR to a directory containing DejaVuSans.ttf and DejaVuSans-Bold.ttf')
    font_css=''.join('@font-face {font-family: "EnvNode Print"; src: url("'+p.as_uri()+'"); font-weight: '+weight+';}' for p,weight in zip(fonts[:2],['400','700']))
    font_css+='@font-face {font-family: "EnvNode Mono"; src: url("'+fonts[2].as_uri()+'");}'
    css=font_css+style.read_text().replace("'DejaVu Sans'", "'EnvNode Print'")
    for name,value in colors.items():css=css.replace('var('+name+')',value)
    env=Environment(loader=FileSystemLoader(str(template.parent)),autoescape=select_autoescape(['html']))
    from markupsafe import Markup
    html=env.get_template(template.name).render(**meta,content=Markup(str(article)),intro=Markup(str(intro)))
    out=site/document['target'];out.parent.mkdir(parents=True,exist_ok=True)
    def local_fetch(url):
        parsed=urlparse(url)
        if parsed.scheme!='file':raise ValueError(f'External PDF asset prohibited: {url}')
        asset=Path(unquote(parsed.path)).resolve()
        if not asset.is_relative_to(site) and asset not in fonts:raise ValueError(f'PDF asset outside build: {url}')
        return default_url_fetcher(url)
    from weasyprint.text.fonts import FontConfiguration
    font_config=FontConfiguration()
    HTML(string=html,base_url=page.as_uri(),url_fetcher=local_fetch).write_pdf(out,stylesheets=[CSS(string=css,font_config=font_config)],font_config=font_config)
    reader=PdfReader(out); text='\n'.join(p.extract_text() or '' for p in reader.pages)
    for required_text in document['required_text']:
        if required_text not in text:raise ValueError(f'PDF content missing: {required_text}')
    if not reader.pages or any(abs(float(p.mediabox.width)-595.276)>1 or abs(float(p.mediabox.height)-841.89)>1 for p in reader.pages):raise ValueError('PDF must use A4')
    original=root/document['original']
    digest=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
    manifest={'generator':'WeasyPrint fixed-template v1','metadata':meta,'pages':len(reader.pages),'source_pdf':document['original'],'source_pdf_sha256':digest(original),'markdown_sha256':digest(source),'template_sha256':digest(template),'print_style_sha256':digest(style),'palette':colors,'renderer_version':__import__('weasyprint').__version__,'generator_sha256':digest(Path(__file__)), 'images':{img.name:digest(img) for img in (root/document['images']).glob('*.png')},'fonts':{p.name:digest(p) for p in fonts},'pdf_sha256':digest(out)}
    out.with_suffix('.json').write_text(json.dumps(manifest,indent=2,ensure_ascii=False)+'\n')
    print(f'PDF pilot: {out} ({len(reader.pages)} pages)')


def render_all(config, documents):
    """Build each explicitly registered guide from its generated HTML article."""
    for document in documents:
        render(config, document)
