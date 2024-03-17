#include "esphome.h"
#include "Adafruit_TSC2007.h"

#define TOUCH_THRESHOLD 50.0

class Tsc2007Sensor: public Component, public Sensor {
  public:
    void set_pin(InternalGPIOPin *pin) { pin_ = pin; }

    Adafruit_TSC2007 touch;

    float get_setup_priority() const override { return esphome::setup_priority::BUS; }

    void setup() override {
      touch.begin();
      pin_->setup();
      pin_->pin_mode(gpio::FLAG_INPUT | gpio::FLAG_PULLUP);
      pin_->attach_interrupt(&Tsc2007Sensor::on_touch_wrapper, this, gpio::INTERRUPT_RISING_EDGE);
      touching = pin_->digital_read();
      // touched_ = new BinarySensor();
      // touched_->publish_initial_state(touching);
      ESP_LOGD("tsc2007", "TSC2007 sensor setup complete.");
    }

    void loop() override {
      uint16_t x, y, z1, z2;
      if (touching) {
        if (touch.read_touch(&x, &y, &z1, &z2)) {
          if(z1 > TOUCH_THRESHOLD) {
            ESP_LOGV("tsc2007", "Touched. x: %u, y: %u, z1: %u", x, y, z1);
            // Only publish state if there is a touch "down" event
            // Swap the TSC2007's concept of x & y for one that matches my expectations
            x_sensor_->publish_state(y);
            y_sensor_->publish_state(x);
            z1_sensor_->publish_state(z1);
            // Ignore z2, it's an inverted reading of touch pressure
            // z2_sensor->publish_state(z2);
          }
        }
      }
    }

    ~Tsc2007Sensor() {
        // Remove the interrupt handler when the object is destroyed
        pin_->detach_interrupt();
    }

    Sensor* get_x_sensor() {
      return x_sensor_;
    }

    Sensor* get_y_sensor() {
      return y_sensor_;
    }

    Sensor* get_pressure_sensor() {
      return z1_sensor_;
    }

    // BinarySensor* get_touched_sensor() {
    //   return touched_;
    // }

  protected: 
    InternalGPIOPin *pin_;
    Sensor *x_sensor_ = new Sensor();
    Sensor *y_sensor_ = new Sensor();
    Sensor *z1_sensor_ = new Sensor();
    // BinarySensor *touched_ = new BinarySensor();
    bool touching = false;

    static void IRAM_ATTR on_touch_wrapper(Tsc2007Sensor* arg) {
      ESP_LOGV("tsc2007", "Touch detected.");
      arg->on_touch();
    }

  private:
    bool current_touching;
    void on_touch() {
      current_touching = pin_->digital_read();
      if(current_touching != touching) {
        touching = current_touching;
        ESP_LOGV("tsc2007", "Touching updated to %d", touching);
      //   // touched_->publish_state(touching);
      }
    }
};

class Tsc2007LightOutput: public Component, public LightOutput {
  public:
    void set_pin(InternalGPIOPin *pin) { pin_ = pin; }

    Adafruit_TSC2007 touch;

    float get_setup_priority() const override { return esphome::setup_priority::BUS; }

    LightTraits get_traits() override {
      // return the traits this light supports
      auto traits = LightTraits();
      traits.set_supported_color_modes({ColorMode::RGB, ColorMode::COLD_WARM_WHITE, ColorMode::BRIGHTNESS});
      return traits;
    }
    void setup() override {
      touch.begin();
      pin_->setup();
      pin_->pin_mode(gpio::FLAG_INPUT | gpio::FLAG_PULLUP);
      pin_->attach_interrupt(&Tsc2007LightOutput::on_touch_wrapper, this, gpio::INTERRUPT_RISING_EDGE);
      touching = pin_->digital_read();
      ESP_LOGD("tsc2007light", "TSC2007 sensor setup complete.");
    }

    void loop() override {
      uint16_t x, y, z1, z2;
      if (touching) {
        if (touch.read_touch(&x, &y, &z1, &z2)) {
          if(z1 > TOUCH_THRESHOLD) {
            ESP_LOGV("tsc2007light", "Touched. x: %u, y: %u, z1: %u", x, y, z1);

            // For the bottom quarter of the panel...            
            if(y < 1000) {
              this->publish_state(!this->is_on())
            } else if(y >= 1000 && y < 2000) {
              // set white color temp
              this->publish_state(LightState().current_values_as_rgbww(0.0, 0.0, 0.0, x/4000.0, 1.0-x/4000.0))
            } else if(y >= 2000 && y < 3000) {
              // set brightness
              this->publish_state(LightState().current_values_as_brightness(x/4000.0))
            } else if(y >= 3000 && y < 4000) {
              // set hue
              float hue = x/4000.0;
              float angle = hue * 360; // Convert hue to angle in degrees

              float rad = angle * PI / 180; // Convert angle to radians

              float red = cos(rad);
              float green = cos(rad + 2 * PI / 3);
              float blue = cos(rad + 4 * PI / 3);

              // Normalize values to range between 0 and 1
              red = (red + 1) / 2;
              green = (green + 1) / 2;
              blue = (blue + 1) / 2;
              this->publish_state(LightState().current_values_as_rgb(red, green, blue))
            }
          }
        }
      }
    }

    ~Tsc2007LightOutput() {
        // Remove the interrupt handler when the object is destroyed
        pin_->detach_interrupt();
    }

  protected: 
    InternalGPIOPin *pin_;
    bool touching = false;

    static void IRAM_ATTR on_touch_wrapper(Tsc2007LightOutput* arg) {
      ESP_LOGV("tsc2007light", "Touch detected.");
      arg->on_touch();
    }

  private:
    bool current_touching;
    void on_touch() {
      current_touching = pin_->digital_read();
      if(current_touching != touching) {
        touching = current_touching;
        ESP_LOGV("tsc2007light", "Touching updated to %d", touching);
      //   // touched_->publish_state(touching);
      }
    }
};
