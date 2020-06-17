#pragma once

#include "esphome/components/climate_ir/climate_ir.h"

const uint8_t PANASONIC_AIRCON2_PROFILE_NORMAL = 0x00;
const uint8_t PANASONIC_AIRCON2_QUIET     = 0x20;
const uint8_t PANASONIC_AIRCON2_POWERFUL  = 0x01;

const uint8_t PANASONIC_AIRCON2_VS_AUTO   = 0x0F;
const uint8_t PANASONIC_AIRCON2_VS_UP     = 0x01;
const uint8_t PANASONIC_AIRCON2_VS_MUP    = 0x02;
const uint8_t PANASONIC_AIRCON2_VS_MIDDLE = 0x03;
const uint8_t PANASONIC_AIRCON2_VS_MDOWN  = 0x04;
const uint8_t PANASONIC_AIRCON2_VS_DOWN   = 0x05;

const uint8_t PANASONIC_AIRCON2_FAN_AUTO = 0xA0;
const uint8_t PANASONIC_AIRCON2_FAN1 = 0x30;
const uint8_t PANASONIC_AIRCON2_FAN2 = 0x40;
const uint8_t PANASONIC_AIRCON2_FAN3 = 0x50;
const uint8_t PANASONIC_AIRCON2_FAN4 = 0x60;
const uint8_t PANASONIC_AIRCON2_FAN5 = 0x70;

namespace esphome {
namespace panasonic {

/// Simple enum to represent models.
enum Model {
  MODEL_SKE = 0, 
};


class PanasonicClimate : public climate_ir::ClimateIR {
 public:
  PanasonicClimate()
      : climate_ir::ClimateIR(temperature_min_(), temperature_max_(), 0.5f, true, true,
                              {climate::CLIMATE_FAN_AUTO, climate::CLIMATE_FAN_LOW, climate::CLIMATE_FAN_MEDIUM,
                               climate::CLIMATE_FAN_HIGH},
                              {climate::CLIMATE_SWING_OFF, climate::CLIMATE_SWING_VERTICAL}) {}

  void setup() override {
    climate_ir::ClimateIR::setup();
	//this->FanSpeed = 0;
	this->VerticalAir = PANASONIC_AIRCON2_VS_MIDDLE;
	this->ignoreTransmit = false;
	this->Profile = PANASONIC_AIRCON2_PROFILE_NORMAL;
	
	switch(this->fan_mode) {
		case climate::CLIMATE_FAN_AUTO:
			this->FanSpeed = PANASONIC_AIRCON2_FAN_AUTO;
			break;
		case climate::CLIMATE_FAN_HIGH:
		    this->FanSpeed = PANASONIC_AIRCON2_FAN5;
		break;
		case climate::CLIMATE_FAN_MEDIUM:
			this->FanSpeed = PANASONIC_AIRCON2_FAN3;
			break;
		case climate::CLIMATE_FAN_LOW:
			this->FanSpeed = PANASONIC_AIRCON2_FAN1;
			break;
	}
  }

  /// Override control to change settings of the climate device.
  void control(const climate::ClimateCall &call) override {
	if(call.get_fan_mode().has_value())
	{
		switch(*call.get_fan_mode())
		{
			case climate::CLIMATE_FAN_AUTO:
				this->FanSpeed = PANASONIC_AIRCON2_FAN_AUTO;
				break;
			case climate::CLIMATE_FAN_HIGH:
			    this->FanSpeed = PANASONIC_AIRCON2_FAN5;
				break;
			case climate::CLIMATE_FAN_MEDIUM:
				this->FanSpeed = PANASONIC_AIRCON2_FAN3;
				break;
			case climate::CLIMATE_FAN_LOW:
				this->FanSpeed = PANASONIC_AIRCON2_FAN1;
				break;
		}
	}
	if(call.get_swing_mode().has_value())
	{
		if(*call.get_swing_mode()==climate::CLIMATE_SWING_VERTICAL)
			this->VerticalAir = PANASONIC_AIRCON2_VS_AUTO;
		else if(*call.get_swing_mode()==climate::CLIMATE_SWING_OFF)
			this->VerticalAir = PANASONIC_AIRCON2_VS_MIDDLE;
	}
	// Disable transmit from paret class to be able to put some safe guards on temperature
	this->ignoreTransmit = true;
    climate_ir::ClimateIR::control(call);
	this->ignoreTransmit = false;
	
	// Make sure temperature is within limits
	if(this->mode == climate::CLIMATE_MODE_HEAT)
	{
		if(this->target_temperature < 8.0)
			this->target_temperature = 8.0;
	}
	else
	{
		if(this->target_temperature < 16.0)
			this->target_temperature = 16.0;
	}
	if(this->target_temperature > 30.0)
		this->target_temperature = 30.0;
	// Transmit and publish
	this->transmit_state();
	this->publish_state();
  }

  void set_model(Model model) { this->model_ = model; }
  
  std::string getModeFormatted()
  {
	switch(this->mode){
		case climate::CLIMATE_MODE_AUTO:
		  return "Auto";
		case climate::CLIMATE_MODE_HEAT:
		  return "Heat";
		case climate::CLIMATE_MODE_COOL:
		  return "Cool";
		case climate::CLIMATE_MODE_DRY:
		  return "Dry";
		case climate::CLIMATE_MODE_FAN_ONLY:
		  return "Fan";
		case climate::CLIMATE_MODE_OFF:
		  return "Off";
		default: 
			return "Unknown";
	}
  }
  
  bool isProfileNormal() { return this->Profile == PANASONIC_AIRCON2_PROFILE_NORMAL;}
  bool isProfilePowerful(){ return this->Profile == PANASONIC_AIRCON2_POWERFUL;}
  bool isProfileQuiet() { return this->Profile == PANASONIC_AIRCON2_QUIET;}

  std::string getVerticalAirFormatted()
  {
	  switch(this->VerticalAir)
	  {
		  case PANASONIC_AIRCON2_VS_AUTO:
			return "Auto";
		  case PANASONIC_AIRCON2_VS_UP:
			return "15°";
		  case PANASONIC_AIRCON2_VS_MUP:
			return "20°";
		  case PANASONIC_AIRCON2_VS_MIDDLE:
			return "30°";
		  case PANASONIC_AIRCON2_VS_MDOWN:
			return "37°";
		  case PANASONIC_AIRCON2_VS_DOWN:
			return "45°";
	      default: 
			return "";
	  }
  }
  
  std::string getFanSpeedFormatted()
  {
	  switch(this->FanSpeed)
	  {
		  case PANASONIC_AIRCON2_FAN_AUTO:
		    return "Auto";
		  case PANASONIC_AIRCON2_FAN1:
		    return "20%";
		  case PANASONIC_AIRCON2_FAN2:
		    return "40%";
		  case PANASONIC_AIRCON2_FAN3:
		    return "60%";
	      case PANASONIC_AIRCON2_FAN4:
		    return "80%";
		  case PANASONIC_AIRCON2_FAN5:
		    return "100%";
		  default:
		   return "";
	  }
  }

 protected:
  /// Transmit via IR the state of this climate controller.
  void transmit_state() override;
  /// Handle received IR Buffer
  bool on_receive(remote_base::RemoteReceiveData data) override;

  Model model_;
  
  uint8_t VerticalAir;
  uint8_t FanSpeed;
  uint8_t Profile;
  bool ignoreTransmit;

  float temperature_min_() {
    return 8.0;
  }
  float temperature_max_() {
    return 30.0;
  }
  
  void debugRemoteBuffer(uint8_t remote_state[]);
};

}  // namespace panasonic
}  // namespace esphome