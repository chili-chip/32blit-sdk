# Multiplayer

`engine/multiplayer.hpp`

Two copies of a game can send each other messages: two desktop builds over the network, or a console and a program on a computer over USB. It's a simple, reliable byte pipe; what the messages mean is up to your game.

## Functions

```cpp
void enable_multiplayer();
void disable_multiplayer();
bool is_multiplayer_connected();

void send_message(const uint8_t *data, uint16_t len);
extern void (*&message_received)(const uint8_t *data, uint16_t len);
```

`enable_multiplayer()`
:   Starts looking for the other player. Multiplayer is off until you call it, so single-player games never touch the network or the USB connection.

`disable_multiplayer()`
:   Disconnects and stops looking.

`is_multiplayer_connected()`
:   `true` once both sides have enabled multiplayer and found each other. Check it before relying on the other player, and keep checking: the connection can drop at any time.

`send_message(data, len)`
:   Sends up to 65 535 bytes. Messages arrive whole and in order. If nobody is connected, the message is dropped.

`message_received`
:   A function pointer, `nullptr` by default. Set it to a function of yours; it's called with each message that arrives, between your updates. The data is only valid during the call, so copy what you need.

## Example

```cpp
struct PlayerState {
  int16_t x, y;
  uint8_t facing;
};

PlayerState me, them;

void on_message(const uint8_t *data, uint16_t len) {
  if(len == sizeof(PlayerState))
    memcpy(&them, data, sizeof(PlayerState));
}

void init() {
  message_received = on_message;
  enable_multiplayer();
}

void update(uint32_t time) {
  move_player(me);

  static uint32_t last_send = 0;
  if(is_multiplayer_connected() && time - last_send >= 50) {  // 20 times a second
    send_message((const uint8_t *)&me, sizeof(me));
    last_send = time;
  }
}
```

Sending a struct's bytes like this works between builds for the same kind of machine. Between the console and a desktop build, the sizes and layout of types can differ, so send fixed-size integers (`int16_t`, `uint8_t`), never pointers. A packed struct (`#pragma pack(push, 1)`) avoids differences in padding. Include a message type byte at the start once you have more than one kind of message.

## On the desktop

Desktop builds connect over TCP on port 12977 (`0x32B1`). By default, `enable_multiplayer()` first tries to connect to another copy on the same computer, and listens for one if there isn't one yet, so starting the game twice links them. To play across a network, choose the roles on the command line:

```bash
./game --listen                 # on the first computer
./game --connect 192.168.1.20   # on the second, with the first one's address
```

If the connection drops, the game retries every few seconds while multiplayer is enabled. Desktop builds need SDL2_net, which the [setup guides](../setup/linux.md) install.

## On the VGC Zero

On the console, messages travel over the USB serial port, using the same message format as the desktop build. The other end has to be a program on the computer that speaks that format; the SDK doesn't include one, so console multiplayer is for your own tools (for example, a PC companion app or a test harness) rather than console-to-console play.

The protocol is short: each side sends `32BLMLTI` followed by one byte (`1` to announce itself, `2` to reply, `0` when leaving), and each message is `32BLUSER`, the length as two bytes (low byte first), then the data.
