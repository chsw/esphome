#include "panasonic.h"
#include "esphome/core/log.h"

namespace esphome {
namespace panasonic {

static const char *TAG = "panasonic.climate";

const uint16_t HEADER_MARK = 3500;
const uint16_t HEADER_SPACE = 1800;
const uint16_t BIT_MARK = 420;
const uint16_t ONE_SPACE = 1350;
const uint16_t ZERO_SPACE = 470;
const uint16_t GAP = 10000;
const uint8_t  STATE_LENGTH = 27;


const uint8_t PANASONIC_AIRCON2_MODE_AUTO = 0x00;
const uint8_t PANASONIC_AIRCON2_MODE_HEAT = 0x40;
const uint8_t PANASONIC_AIRCON2_MODE_COOL = 0x30;
const uint8_t PANASONIC_AIRCON2_MODE_DRY = 0x20;
const uint8_t PANASONIC_AIRCON2_MODE_FAN = 0x60;

const uint8_t PANASONIC_AIRCON2_FAN_AUTO = 0xA0;
const uint8_t PANASONIC_AIRCON2_FAN1 = 0x30;
const uint8_t PANASONIC_AIRCON2_FAN2 = 0x40;
const uint8_t PANASONIC_AIRCON2_FAN3 = 0x50;
const uint8_t PANASONIC_AIRCON2_FAN4 = 0x60;
const uint8_t PANASONIC_AIRCON2_FAN5 = 0x70;

const uint8_t PANASONIC_AIRCON2_VS_AUTO   = 0x0F;
const uint8_t PANASONIC_AIRCON2_VS_UP     = 0x01;
const uint8_t PANASONIC_AIRCON2_VS_MUP    = 0x02;
const uint8_t PANASONIC_AIRCON2_VS_MIDDLE = 0x03;
const uint8_t PANASONIC_AIRCON2_VS_MDOWN  = 0x04;
const uint8_t PANASONIC_AIRCON2_VS_DOWN   = 0x05;

void PanasonicClimate::transmit_state() {
	if(this->ignoreTransmit)
	{
		ESP_LOGV(TAG, "Ignoring transmit");
		return;
	}
	ESP_LOGD(TAG, "Transmitting state: ");
	
	// IR template from NZ9SKE remote 
	uint8_t remote_state[STATE_LENGTH] = 
   
	{
		0x02, 0x20, 0xE0, 0x04, 0x00, 0x00, 0x00, 0x06, 0x02, 0x20, 0xE0, 0x04, 0x00, 0x08, 0x00, 0x80, 0x00, 0x00, 0x00, 0x0E, 0xE0, 0x00, 0x00, 0x89, 0x00, 0x00, 0x00
		//0     1     2     3     4     5     6     7     8     9    10    11    12    13    14   15     16    17    18    19    20    21    22    23    24    25    26
	}; 

	// Set power
	remote_state[13] = (remote_state[13] &0xFE)|(this->mode == climate::CLIMATE_MODE_OFF ? 0x00 : 0x01);
  
	switch (this->mode) {
		case climate::CLIMATE_MODE_AUTO:
			remote_state[13] = (remote_state[13]&0xF)|PANASONIC_AIRCON2_MODE_AUTO;
			break;
		case climate::CLIMATE_MODE_OFF:
			// Need a valid mode even if transmitting power off
		case climate::CLIMATE_MODE_HEAT:
			remote_state[13] = (remote_state[13]&0xF)|PANASONIC_AIRCON2_MODE_HEAT;
			break;
		case climate::CLIMATE_MODE_COOL:
			remote_state[13] = (remote_state[13]&0xF)|PANASONIC_AIRCON2_MODE_COOL;
			break;
		case climate::CLIMATE_MODE_DRY:
			remote_state[13] = (remote_state[13]&0xF)|PANASONIC_AIRCON2_MODE_DRY;
			break;
		case climate::CLIMATE_MODE_FAN_ONLY:
			remote_state[13] = (remote_state[13]&0xF)|PANASONIC_AIRCON2_MODE_FAN;
			break;
		default:
	}

	// Temperature
	remote_state[14]=((int)(clamp(this->target_temperature, this->temperature_min_(), this->temperature_max_())*2.0f));

	// Fan speed
	// TODO hantera övriga states
	if(this->target_temperature < 16.0)
	{
		remote_state[16] = (remote_state[16]&0xF)|PANASONIC_AIRCON2_FAN5;
	}
	else
	{
		remote_state[16] = (remote_state[16]&0xF)|this->FanSpeed;
	}

	// Swing
	if(this->VerticalAir != 0)
	{
		remote_state[16] = (remote_state[16] & 0xF0) | this->VerticalAir;
	}
	else
	{
		switch(this->swing_mode) {
			case climate::CLIMATE_SWING_VERTICAL:
				remote_state[16] = (remote_state[16] & 0xF0) | PANASONIC_AIRCON2_VS_AUTO;
				break;
			default:
				remote_state[16] = (remote_state[16] & 0xF0) | PANASONIC_AIRCON2_VS_MIDDLE;
				// TODO
		  //remote_state[16] = (remote_state[16] & 0xF0) | PANASONIC_AIRCON2_VS_UP;
		  //remote_state[16] = (remote_state[16] & 0xF0) | PANASONIC_AIRCON2_VS_MUP;
		  //remote_state[16] = (remote_state[16] & 0xF0) | PANASONIC_AIRCON2_VS_MIDDLE;
		  //remote_state[16] = (remote_state[16] & 0xF0) | PANASONIC_AIRCON2_VS_MDOWN;
		  //remote_state[16] = (remote_state[16] & 0xF0) | PANASONIC_AIRCON2_VS_DOWN;
		  break;
	  }
	}
  
	// Profile
	remote_state[21] = target_temperature < 16.0 ? PANASONIC_AIRCON2_PROFILE_NORMAL : this->Profile;

	// Create checksum                                                                                                                                                                 
	uint8_t checksum = 0x06; // found differce, so start with 0x06                                                                                                                        
	for (int i = 13; i < 26; i++){
		checksum = checksum + remote_state[i];
	}
	checksum = (checksum & 0xFF); // mask out only first byte)                                                                                                                            
	ESP_LOGV(TAG, "Calculated checksum: 0x%x", checksum);
	remote_state[26] = checksum;

	// print info about what gets transmitted
	this->debugRemoteBuffer("Transmitting", remote_state);

	// Create bit-pattern for transmit
	auto transmit = this->transmitter_->transmit();
	auto data = transmit.get_data();

	data->set_carrier_frequency(38000);

	// Header
	data->mark(HEADER_MARK);
	data->space(HEADER_SPACE);
  
	// Data
	auto bytes_sent = 0;
	for (uint8_t i : remote_state) {
		for (uint8_t j = 0; j < 8; j++) {
			data->mark(BIT_MARK);
			bool bit = i & (1 << j);
			data->space(bit ? ONE_SPACE : ZERO_SPACE);
		}
		bytes_sent++;
		if (bytes_sent == 8 ) {
			// Divider
			data->mark(BIT_MARK);
			data->space(GAP);
			data->mark(HEADER_MARK);
			data->space(HEADER_SPACE);
		}
	}
	// Footer
	data->mark(BIT_MARK);

	// Transmit
	transmit.perform();
}

bool PanasonicClimate::on_receive(remote_base::RemoteReceiveData data) {
	// Validate header
	ESP_LOGV(TAG, "on_receive");
	if (!data.expect_item(HEADER_MARK, HEADER_SPACE)) {
		ESP_LOGV(TAG, "Header fail");
		return false;
	}

	uint8_t remote_state[STATE_LENGTH] = {0};
	// Read all bytes.
	for (int i = 0; i < STATE_LENGTH; i++) {
		// Read bit
		if (i == 8 /*|| i == 14*/) {
			ESP_LOGV(TAG, "Byte 8: next 3: %d %d %d %d", data.peek(0), data.peek(1), data.peek(2), data.peek(3));
			if (!data.expect_item(BIT_MARK, GAP))
			{
				ESP_LOGV(TAG, "Expected bit mark and gap at Byte %d", i);
				return false;
			}
			if(!data.expect_item(HEADER_MARK, HEADER_SPACE))
			{
				ESP_LOGV(TAG, "Expected header mark+space before byte 8");
				return false;
			}
		}
		for (int j = 0; j < 8; j++) {
			if (data.expect_item(BIT_MARK, ONE_SPACE))
			{
				remote_state[i] |= 1 << j;
			}
			else if (!data.expect_item(BIT_MARK, ZERO_SPACE)) {
				ESP_LOGV(TAG, "Byte %d bit %d fail", i, j);
				return false;
			}
		}
		ESP_LOGV(TAG, "Byte %d %02X", i, remote_state[i]);
	}
	// Print debug info about received buffer
	this->debugRemoteBuffer("Received", remote_state);


	// Check checksum                                                                                                                                                                 
	// Assumption: bitwise sum of payload bytes (so ignore header, and last byte)                                                                                                         
	uint8_t checksum = 0x06; // found differce, so start with 0x06                                                                                                                        
	for (int i = 13; i < 26; i++){
		checksum = checksum + remote_state[i];
	}
	checksum = (checksum & 0xFF); // mask out only first byte)                                                                                                                            
	ESP_LOGV(TAG, "Calculated checksum: 0x%x", checksum);
	if(checksum == remote_state[26])
		ESP_LOGD(TAG, "Checksum OK");
	else
	{
		ESP_LOGD(TAG, "Checksum not OK. Should be: 0x%x", remote_state[26]);
		return false;
	}

	// verify header remote code
	if (remote_state[0] != 0x02 || remote_state[1] != 0x20 || remote_state[2] != 0xE0 || remote_state[3] != 0x04)
	{
		ESP_LOGV(TAG, "Header verification failed!");
		return false;
	}

	// powr on/off button
	ESP_LOGV(TAG, "Power: %02X", (remote_state[13] & 0x01));
	bool power_on = (remote_state[13] & 0x01) == 0x01;
  
	if(power_on == false){
		this->mode = climate::CLIMATE_MODE_OFF;
	}
	else {
		auto mode = remote_state[13] & 0xF0;
		ESP_LOGV(TAG, "Mode: %02X", mode);
		switch (mode) {
			case PANASONIC_AIRCON2_MODE_HEAT:
				this->mode = climate::CLIMATE_MODE_HEAT;
				break;
			case PANASONIC_AIRCON2_MODE_COOL:
				this->mode = climate::CLIMATE_MODE_COOL;
				break;
			case PANASONIC_AIRCON2_MODE_DRY:
				this->mode = climate::CLIMATE_MODE_DRY;
				break;
			case PANASONIC_AIRCON2_MODE_FAN:
				this->mode = climate::CLIMATE_MODE_FAN_ONLY;
				break;
			case PANASONIC_AIRCON2_MODE_AUTO:
				this->mode = climate::CLIMATE_MODE_AUTO;
				break;
			// todo DEFAULT
		}
	}
	
	// Set received temp
	ESP_LOGVV(TAG, "Temperature Raw: %02X", remote_state[14]);
	float temp = ((int)remote_state[14])/2.0f;
	ESP_LOGV(TAG, "Temperature Climate: %f", temp);
	this->target_temperature = temp;

	// Set received fan speed
	auto fan = remote_state[16] & 0xF0;
	ESP_LOGVV(TAG, "Fan: %02X", fan);
	
	// TODO fan mode is truncated
	this->FanSpeed = fan;
	switch (fan) {
		case PANASONIC_AIRCON2_FAN5:
		case PANASONIC_AIRCON2_FAN4:
			this->fan_mode = climate::CLIMATE_FAN_HIGH;
			break;
		case PANASONIC_AIRCON2_FAN3:
		case PANASONIC_AIRCON2_FAN2:
			this->fan_mode = climate::CLIMATE_FAN_MEDIUM;
			break;
		case PANASONIC_AIRCON2_FAN1:
			this->fan_mode = climate::CLIMATE_FAN_LOW;
			break;
		case PANASONIC_AIRCON2_FAN_AUTO:
		default:
			this->fan_mode = climate::CLIMATE_FAN_AUTO;
			break;
	}

	// Set received swing status
	auto verticalAir = remote_state[16] & 0x0F;
	ESP_LOGV(TAG, "Vertical air: %02X", verticalAir);
	this->VerticalAir = verticalAir;
	switch(verticalAir)
	{
		case PANASONIC_AIRCON2_VS_AUTO:
			this->swing_mode = climate::CLIMATE_SWING_VERTICAL;
			break;
		case PANASONIC_AIRCON2_VS_UP:
		case PANASONIC_AIRCON2_VS_MUP:
		case PANASONIC_AIRCON2_VS_MIDDLE:
		case PANASONIC_AIRCON2_VS_MDOWN:
		case PANASONIC_AIRCON2_VS_DOWN:
			this->swing_mode = climate::CLIMATE_SWING_OFF;
	}
  
    // Parse profile
	this->Profile = remote_state[21];
	ESP_LOGV(TAG, "Profile: %02X", this->Profile);
  
	this->publish_state();
	return true;
}

void PanasonicClimate::debugRemoteBuffer(std::string prefix, uint8_t *remote_state)
{
	// Print actual values
	ESP_LOGV(
		TAG,
		"%s: %02X %02X %02X %02X   %02X %02X %02X %02X   %02X %02X %02X %02X   %02X %02X %02X %02X   %02X %02X "
		"%02X %02X   %02X %02X %02X %02X   %02X %02X %02X", prefix.c_str(),
		remote_state[0], remote_state[1], remote_state[2], remote_state[3], remote_state[4], remote_state[5],
		remote_state[6], remote_state[7], remote_state[8], remote_state[9], remote_state[10], remote_state[11],
		remote_state[12], remote_state[13], remote_state[14], remote_state[15], remote_state[16], remote_state[17],
		remote_state[18], remote_state[19], remote_state[20], remote_state[21], remote_state[22], remote_state[23],
		remote_state[24], remote_state[25], remote_state[26]
	);
	// Power  
	ESP_LOGD(TAG, "%s Power: %s", prefix.c_str(), ((remote_state[13] & 0x01) == 0x01)?"on":"off");
	// Mode
	auto str = "";
	switch(remote_state[13] & 0xF0) {
      case PANASONIC_AIRCON2_MODE_HEAT:
        str = "Heat";
        break;
      case PANASONIC_AIRCON2_MODE_COOL:
        str = "Cool";
        break;
      case PANASONIC_AIRCON2_MODE_DRY:
        str = "Dry";
        break;
      case PANASONIC_AIRCON2_MODE_FAN:
        str = "Fan";
        break;
      case PANASONIC_AIRCON2_MODE_AUTO:
        str = "Auto";
        break;
	  default: 
	    str = "Unknown";
    }
	ESP_LOGD(TAG, "%s Mode: %s", prefix.c_str(), str);
	// Away/Maintenance
	ESP_LOGD(TAG, "%s Away: %s", prefix.c_str(), (remote_state[14] & 0x20) ? "No" : "Yes");
	// Temperature
    ESP_LOGD(TAG, "%s Temperature: %f", prefix.c_str(), ((int)remote_state[14])/2.0f);
	// Fan speed
	switch (remote_state[16] & 0xF0) {
		case PANASONIC_AIRCON2_FAN5:
		  str = "Fan5";
		  break;
		case PANASONIC_AIRCON2_FAN4:
		  str = "Fan4";
		  break;
		case PANASONIC_AIRCON2_FAN3:
		  str = "Fan3";
		  break;
		case PANASONIC_AIRCON2_FAN2:
		  str = "Fan2";
		  break;
		case PANASONIC_AIRCON2_FAN1:
		  str = "Fan1";
		  break;
		case PANASONIC_AIRCON2_FAN_AUTO:
		  str = "Auto";
		  break;
		default:
		  str = "Unknown";
	}
	ESP_LOGD(TAG, "%s Fan: %s", prefix.c_str(), str);
	
	// Vertical air
	switch(remote_state[16] & 0x0F)
	{
	  case PANASONIC_AIRCON2_VS_AUTO:
		str = "VS_AUTO";
		break;
	  case PANASONIC_AIRCON2_VS_UP:
		str = "VS_UP";
		break;
	  case PANASONIC_AIRCON2_VS_MUP:
	    str = "VS_MUP";
		break;
	  case PANASONIC_AIRCON2_VS_MIDDLE:
	    str = "VS_MIDDLE";
		break;
	  case PANASONIC_AIRCON2_VS_MDOWN:
	    str = "VS_MDOWN";
		break;
	  case PANASONIC_AIRCON2_VS_DOWN:
		str = "VS_DOWN";
		break;
	  default:
	    str = "Unknown";
	}
	ESP_LOGD(TAG, "%s Vertical air: %s", prefix.c_str(), str);
	// Profile
	switch(remote_state[21])
	{
		case PANASONIC_AIRCON2_PROFILE_NORMAL:
			str = "Normal";
			break;
		case PANASONIC_AIRCON2_POWERFUL:
			str = "Powerful";
			break;
		case PANASONIC_AIRCON2_QUIET:
			str = "Quiet";
			break;
		default:
			str = "Unknown";
	}
	ESP_LOGD(TAG, "%s Profile: %s", prefix.c_str(), str);
  
	// Check checksum                                                                                                                                                                 
	uint8_t checksum = 0x06; // found differce, so start with 0x06                                                                                                                        
	for (int i = 13; i < 26; i++){
		checksum = checksum + remote_state[i];
	}
	checksum = (checksum & 0xFF); // mask out only first byte)                                                                                                                            
	ESP_LOGD(TAG, "%s Checksum: 0x%2x (%s)", prefix.c_str(), checksum, (checksum == remote_state[26]) ? "OK" : "Not OK");
}
}  // namespace panasonic
}  // namespace esphome