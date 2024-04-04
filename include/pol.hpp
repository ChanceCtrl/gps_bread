#ifndef pol
#define pol

class polButton {
private:
  unsigned long time_pressed; // time sense first pressed
  float threshhold;           // time that needs to pass to count as pressed
  int pin;                    // pin its on
  uint8_t (*method)(uint8_t); // The function to read the pin's state
  bool pull_down;             // If it goes from 1 to 0 or from 0 to 1

public:
  bool state;
  bool held;
  void init(uint8_t target_pin, float time_req, uint8_t (*read_method)(uint8_t),
            bool is_pull_down);
  void update();
};

// My linter hates this lol
void polButton::init(uint8_t target_pin, float time_req,
                     uint8_t (*read_method)(uint8_t), bool is_pull_down) {
  pin = target_pin;
  threshhold = time_req;
  method = read_method;
  pull_down = is_pull_down;
}

void polButton::update() {
  if ((method(pin) == 1 && !pull_down) || (method(pin) == 0 && pull_down)) {
    if (time_pressed == 0) {
      time_pressed = millis();
    } else if (state) {
      held = true;
    } else {
      if (millis() - time_pressed >= threshhold) {
        state = true;
        held = false;
      }
    }
  } else {
    state = false;
    held = false;
    time_pressed = 0;
  }
}

#endif
