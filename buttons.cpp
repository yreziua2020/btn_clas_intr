#ifndef INTR_EXCLUSIIVE
//#include <functional>
#include <FunctionalInterrupt.h>
#endif
//#include <FunctionalInterrupt.h>
#include "buttons.h"

uint8_t Buttons::add(uint8_t pin, bool level) {
  uint8_t result;
  button_t b;

  if (pin > 15)
    return ERR_INDEX;
  b.pin = pin;
  b.level = level;
  b.paused = false;
  b.pressed = 0;
  result = List<button_t, 10>::add(b);
  if (result != ERR_INDEX) {
    pinMode(pin, level ? INPUT : INPUT_PULLUP);

    attachInterrupt(pin, [this]() { this->_isr(this); }, CHANGE);

  }

  return result;
}

void Buttons::pause(uint8_t index) {
  if (_items && (index < _count)) {
    _items[index].paused = true;

    detachInterrupt(_items[index].pin);

  }
}

void Buttons::resume(uint8_t index) {
  if (_items && (index < _count)) {
    _items[index].paused = false;

    attachInterrupt(_items[index].pin, [this]() { this->_isr(this); }, CHANGE);

  }
}

void ICACHE_RAM_ATTR Buttons::_isr(Buttons *_this) {


  if (_this->_items && _this->_count) {
    uint32_t time = millis() - _this->_isrtime;
    uint32_t inputs = GPI;

    for (uint8_t i = 0; i < _this->_count; ++i) {
      if (((inputs >> _this->_items[i].pin) & 0x01) == _this->_items[i].level) { // Button pressed
        if (_this->_items[i].pressed) { // Was pressed
          if (_this->_items[i].pressed + time < 0xFFFF)
            _this->_items[i].pressed += time;
          else
            _this->_items[i].pressed = 0xFFFF;
        } else {
          _this->_items[i].pressed = 1;
          if (! _this->_items[i].paused)
            _this->onChange(BTN_PRESSED, i);
        }
      } else { // Button released
        if (_this->_items[i].pressed) { // Was pressed
          if (! _this->_items[i].paused) {
            if (_this->_items[i].pressed + time < 0xFFFF)
              _this->_items[i].pressed += time;
            else
              _this->_items[i].pressed = 0xFFFF;
            if (_this->_items[i].pressed >= LONGCLICK_TIME) {
              _this->onChange(BTN_LONGCLICK, i);
            } else if (_this->_items[i].pressed >= CLICK_TIME) {
              _this->onChange(BTN_CLICK, i);
            } else {
              _this->onChange(BTN_RELEASED, i);
            }
          }
          _this->_items[i].pressed = 0;
        }
      }
    }
  }

  _this->_isrtime = millis();
}

void Buttons::cleanup(void *ptr) {

  detachInterrupt(((button_t*)ptr)->pin);

}

bool Buttons::match(uint8_t index, const void *t) {
  if (_items && (index < _count)) {
    return (_items[index].pin == ((button_t*)t)->pin);
  }

  return false;
}

void QueuedButtons::onChange(buttonstate_t state, uint8_t button) {
  event_t e;

  e.state = state;
  e.button = button;

  _events.put(&e, true);
}
