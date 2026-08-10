#pragma once

namespace EnvNode {

enum class DiscoveryRepublishResult {
    Published,
    MqttUnavailable,
    PublishFailed,
};

class IDiscoveryPublisher {
public:
    virtual ~IDiscoveryPublisher() = default;
    virtual void loop() = 0;
    virtual DiscoveryRepublishResult republish() = 0;
};

} // namespace EnvNode
