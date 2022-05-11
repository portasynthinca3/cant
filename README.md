# CAN'T
**CAN Alternative, Not Terrible** (WIP) is a shared decentralized bus intended for use in DIY smart homes with cheap MCUs.
  - **PHY** level: wired, asynchronous, half-duplex. AVR implementation tested with **100kbps** @8MHz (internal RC oscillator). 
  - **APP** level: up to 256 devices, devices react to others' state change broadcasts, as opposed to direct control messages.

## Directories
  - `avr`: sample AVR implementation
  - `tmp_sensor`: KiCad project for a DS18B20-based temperature sensor

## Progress
  - [x] PHY specification laid out
  - [x] APP specification laid out
  - [x] AVR transmitter
  - [ ] AVR receiver
  - [ ] AVR bridge to USB
  - [ ] ESP32 bridge to WiFi
  - [ ] Mobile app
  - [ ] PHY specification consolidated
  - [ ] APP specification consolidated

## PHY specification
Properly terminated and biased RS-485 twisted pair.

### States
The line has three states:
  - **`L`**: One device drives the line, D+ is low, D- is high.
  - **`H`**: One device drives the line, D+ is high, D- is low.
  - **`Z`**: No devices drive the line, biasing must force this state into an **`L`**.

Data is encoded using NRZI:
  - **`T`**: **`L`**->**`H`** or **`H`**->**`L`** (transition), signals a logic **`1`**.
  - **`N`**: **`L`**->**`L`** or **`H`**->**`H`** (no transition), signals a logic **`0`**.

Special conditions:
  - **`NNNNNN`** (no transition for 6 bit times) signals the end of a transmission.
  - Four consecutive **`N`** s must be followed by an additional **`T`** (bit stuffing), even if one is already coming up, to maintain synchronization.

### Frame
An individual transmission is called a **frame**. Frames include, in order:
  - **Sync signal**: An **`HL`** at the rate of the transmission, which it has to maintain during the rest of the frame. For a 100kbps transmission the bit time is 10 microseconds.
  - **Data**: NRZI-encoded with bit stuffing, as explained above. The length (in bits) must be divisible by 8. Bits within bytes are sent MSB first, LSB last.
  - **Checksum**: encoded just like the data. Reference implementation in `avr/src/cant.c` in function `cant_crc`.
  - **Post-transmission silence**: **`NNNNNN`** (no transition for 6 bit times), signifies the end of the transmission.
  - **Release**: **`ZZ`**, the transmitter releases the bus, allowing the biasing network to force it into an **`L`**. Other transmitters aren't allowed to begin transmitting during this time interval.

### Example transmission
```
|X  XXX     X XXX  XX XX XXXX XX XXXX XX X XXX     XXXX     X X XX  XXXXXX  | `H`s marked with an X
|HLLHHHLLLLLHLHHHLLHHLHHLHHHHLHHLHHHHLHHLHLHHHLLLLLHHHHLLLLLHLHLHHLLHHHHHHZZ|
|  NTNNTNNNNTTTNNTNTNTTNTTNNNTTNTTNNNTTNTTTTNNTNNNNTNNNTNNNNTTTTTNTNTNNNNN  |
|  010010000-11001010110110001101100011011110010000-00010000-111101010      | stuffed bits marked with a hyphen
|  01001000         01101100        01101111         0010000 1              | bytes
|          0 1100101        01101100        0010000 0         11101010      | bytes
|      H       e       l        l      o     <space>     !     (CRC8)       | ASCII
```

### Congestion resolution
Since there's only one shared bus, only one device can talk at the same time. Here's the congestion resolution algorithm:
  - Before considering transmission, pick the priority (**`0`** - normal, **`1`** - low). This value is internal to the device and never shared on the bus. Each device should also have a pre-configured constant 8-bit address.
  - Just before taking control of the bus, check time since the last **`T`** state. It must be >= 8 bits at the speed of the last transmission, or the default configured speed if none occurred beforehand. If the time is less than 8 bits, the current transmission has not yet been completed, and the transmitter should wait until it completes.
  - Wait for `(address*(priority+1))/4` bits. It can be observed that the lower the address and higher the priority, the sooner the device will try to write again. It is therefore a good idea to give important devices like intrusion detectors low address numbers.
  - If the line is still busy, wait again.
  - Otherwise, start transmitting.

## APP specification
(all multibyte fields are big endian)

### First three bytes
  - `[0]`: source (transmitter) address
  - `[1]`: 5 MSB = packet type, rest are reserved
  - `[2]`: target address for targeted packet types, any value otherwise

Sections below mention the packet types by their name, numeric value and whether they're (B)roadcast or (T)argeted.

### State Change (0B), State Transmit (1B), Force Switch (4T)
These three packet types use the same field format, but the semantics are different:
  - `State Change` is broadcast every time the state changes.
  - `State Transmit` is broadcast at regular intervals irregardless of changes in the state, preferably about every 1 to 5 seconds.
  - `Force Switch` is sent from one device to another asking for a force state switch, overriding its internal rules. If the change was successful, the target should broadcast a `State Change`.

Fields:
  - `[3]`: 5 MSB = device type, 3 LSB = configuration parameter related to type. Device types:
    - `0`: Switch or switch group; `parameter`: number of switches; `[4]`: switch states, arranged from LSB for switch 0 to MSB for switch 7.
    - `1`: Brightness controller; `[4]`: value.
    - `2`: Light bulb; `parameter[LSB]`: supports brightness control; `[4]`: brightness. For on-off lamps, values above 128 are interpreted as "turned on", and "turned off" for values below 128.
    - `3`: Temperature sensor; `[5:4]`: centidegrees celsius above -273C

### Reset Request (2T)
Requests a device reset.
`[3][LSB]`: enter bootloader for firmware download?

### Program Block (3T)
Piece of firmware to be flashed by the bootloader.
`[4:3]`: page number, interpretation depends on device architecture.
`rest`: page data.

### Program Response (5T)
Sent by the device in bootloader mode in response to a `Program Block`.
`[3]`: status:
  - `0`: OK, page flashed
  - `1`: transmission error
  - `2`: write failed
  - `3`: page missed
`[5:4]` if status==3: first missed page number
