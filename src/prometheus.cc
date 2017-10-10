#include <sstream>
#include <prometheus/exposer.h>
#include <prometheus/registry.h>

extern "C" int c_main(int, char**);

int main(int argc, char **argv) {
    return c_main(argc, argv);
}

namespace {
using namespace prometheus;

template <typename T> class AutoMetrics {
public:
    AutoMetrics(Family<T>* family);
    T& get(std::string key, std::map<std::string, std::string>& labels);
private:
    Family<T> *family_;
    std::map<std::string, T*> metrics_;
};

template <typename T>
AutoMetrics<T>::AutoMetrics(Family<T>* family) : family_(family) {}

template <typename T>
T& AutoMetrics<T>::get(std::string key, std::map<std::string, std::string>& labels) {
    auto search = metrics_.find(key);
    if (search != metrics_.end()) {
        return *search->second;
    }
    T& t = family_->Add(labels);
    metrics_.insert(std::make_pair(key, &t));
    return t;
}

struct Exporter {
    Exporter(std::string);

    Exposer* exposer_;
    std::shared_ptr<Registry> registry_;

    AutoMetrics<Counter> updates_;
    AutoMetrics<Gauge> temperature_;
    AutoMetrics<Gauge> humidity_;
};

static Exporter* e;

extern "C" void prom_setup(int port) {
    std::stringstream bind;
    bind << "0.0.0.0:" << port;
    e = new Exporter(bind.str());
}

Exporter::Exporter(std::string bind_address) :
        exposer_(new Exposer(bind_address)),
        registry_(std::make_shared<Registry>()),
        updates_(AutoMetrics<Counter>(&BuildCounter()
            .Name("updates")
            .Help("count of device updates received")
            .Register(*registry_))),
        temperature_(&BuildGauge()
            .Name("temperature")
            .Help("temperature in device-reported degrees")
            .Register(*registry_)),
        humidity_(&BuildGauge()
            .Name("humidity")
            .Help("humidity percentage")
            .Register(*registry_)) {
    exposer_->RegisterCollectable(registry_);
}

extern "C" void prom_ambient_weather(const char *model, int device_id, int channel, int battery_low, double temperature, int humidity) {
    std::stringstream device_id_, channel_;
    device_id_ << device_id;
    channel_ << channel;
    std::map<std::string, std::string> labels({
        {"model", model},
        {"device_id", device_id_.str()},
        {"channel", channel_.str()}});
    auto key = std::accumulate(
        labels.begin(), labels.end(), std::string{},
        [](const std::string& acc,
           const std::pair<std::string, std::string>& label_pair) {
          return acc + '|' + label_pair.first + '=' + label_pair.second;
        });
    e->updates_.get(key, labels).Increment();
    e->temperature_.get(key, labels).Set(temperature);
    e->humidity_.get(key, labels).Set(humidity);
}
}
