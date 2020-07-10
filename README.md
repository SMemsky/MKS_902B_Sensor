# MKS 902B Pressure sensor protocol

## Build and run

`build`

## Example usage

```
MKS_902B dev;
// MKS_902B::Torr
// MKS_902B::MilliBar
// MKS_902B::Pascal
if (dev.initialize(253, MKS_902B::Torr) != MKS_902B::Result::Success) {
	std::cout << "Failed dev initialize" << std::endl;
	return 1;
}

// Configure setpoint 1, with ON state when pressure is above 50.0 units
// switches into OFF state when pressure is below 50.0 units by 10.0 units
// true to enable setpoint right away
// false to disable
// MKS_902B::Above
// MKS_902B::Below
if (dev.configureSetpoint(1, 40.0, 10.0, MKS_902B::Above, true) != MKS_902B::Result::Success) {
	return 1;
}
// dev.enableSetpoint();
// dev.disableSetpoint();

float pressure = 0.0;
bool  status   = false;
while (true) {
	dev.getPressure(pressure);
	dev.getSetpointStatus(1, status);
	std::cout << pressure << " " << status << std::endl;
}
```
