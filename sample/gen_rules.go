package main

import (
	"os"
	"text/template"
)

type sensor struct {
	Channel, DeviceID int
	Location string
}

var (
	sensors = []sensor{
		{1, 159, "master bedroom"},
		{2,  87, "office balcony"},
		{3, 198, "office"},
		{4, 237, "attic"},
		{5, 221, "library"},
		{6, 192, "family room"},
	}

)

const rules = `
updates:rate5m = rate(updates{job="rtl433"}[5m])

{{ range . }}
location_temperature{location="{{ .Location }}",channel="",device_id="",instance="",model=""} = temperature{channel="{{ .Channel }}",device_id="{{ .DeviceID }}",model="Ambient Weather F007TH Thermo-Hygrometer"} and updates:rate5m > 0
location_humidity{location="{{ .Location }}",channel="",device_id="",instance="",model=""} = humidity{channel="{{ .Channel }}",device_id="{{ .DeviceID }}",model="Ambient Weather F007TH Thermo-Hygrometer"} and updates:rate5m > 0
{{ end }}`

func main() {
	template.Must(template.New("").Parse(rules)).Execute(os.Stdout, sensors)
}
