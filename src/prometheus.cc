#include <sstream>
#include <prometheus/exposer.h>
#include <prometheus/registry.h>

extern "C" int c_main(int, char**);

int main(int argc, char **argv) {
    return c_main(argc, argv);
}

namespace {
using namespace prometheus;

class Exporter {
public:
    Exporter(std::string);
    Gauge& temperature(std::map<std::string, std::string> labels);

private:
    Exposer* exposer_;
    std::shared_ptr<Registry> registry_;

    Family<Gauge> *temperature_family_;
    // TODO(dichro): why can't I retrieve metrics from a Family?
    std::map<std::string, Gauge*> temperatures_;
};

static Exporter* e;

extern "C" void prom_setup(int port) {
    std::stringstream bind;
    bind << "0.0.0.0:" << port;
    e = new Exporter(bind.str());
}

Exporter::Exporter(std::string bind_address) :
        exposer_(new Exposer(bind_address)),
        registry_(std::make_shared<Registry>()) {
    temperature_family_ = &BuildGauge()
            .Name("temperature")
            .Help("temperature in device-reported degrees")
            .Register(*registry_);
    exposer_->RegisterCollectable(registry_);
}

Gauge& Exporter::temperature(std::map<std::string, std::string> labels) {
    auto combined = std::accumulate(
        labels.begin(), labels.end(), std::string{},
        [](const std::string& acc,
           const std::pair<std::string, std::string>& label_pair) {
          return acc + '|' + label_pair.first + '=' + label_pair.second;
        });
    auto search = temperatures_.find(combined);
    if (search != temperatures_.end()) {
        return *search->second;
    }
    Gauge& t = temperature_family_->Add(labels);
    temperatures_.insert(std::make_pair(combined, &t));
    return t;
}

extern "C" void prom_ambient_weather(const char *model, int device_id, int channel, int battery_low, double temperature, int humidity) {
    std::stringstream device_id_, channel_;
    device_id_ << device_id;
    channel_ << channel;
    std::map<std::string, std::string> key({
        {"model", model},
        {"device_id", device_id_.str()},
        {"channel", channel_.str()}});
    e->temperature(key).Set(temperature);
}
}
