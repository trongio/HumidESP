# How much humidifier does the apple room need?

Rough numbers to pick parts. The interactive version is at the top of [shopping.html](shopping.html).

## Where the moisture goes

Air at 0-4 °C holds very little water: about **5.6 g/m³ when saturated** at 2 °C. Going from 85 % to 95 %RH only adds 0.6 g/m³,
so filling the room's air once is trivial (about 50 g for 90 m³). What keeps drying the room is:

1. **The cooler's evaporator.** Its coil runs a few degrees below zero, so moisture freezes onto it all the time.
   In a small store the latent part of the cooling load is roughly 0.1-0.4 kW, which is **about 150-500 g of water per hour** taken out of the air.
2. **Air leaks and door openings**, which swap moist air for drier air.
3. Apples add some moisture back (they transpire), which is the weight loss we are trying to stop.

## Rule of thumb

| | Per m³ of room | 25-30 m² × 3 m (75-90 m³) |
|---|---|---|
| Typical need | about 4 ml/h per m³ | 0.3-0.36 L/h |
| **Capacity to install** (×2 margin: cold mist evaporates slowly, door openings, loading) | **about 8 ml/h per m³** | **0.6-0.7 L/h** |
| Water used | | about 3-8 L per day |

The controller switches humidifiers on and off, so oversizing by 2-3× is fine. Much more than that and the mist wets the crates before it evaporates.

## What gives how much

| Humidifier | Output (approx.) | Power | Price |
|---|---|---|---|
| JY-M22A0 board with 4 small 5 V discs | 0.1-0.2 L/h | 6-10 W at 5 V | the one you have |
| 24 V ultrasonic fogger, 1 head, with adapter | about 0.3 L/h | 15-20 W | $10.29 (AliExpress app, Oct 2026) |
| 24 V fogger, 10 heads | about 3 L/h | about 250 W | about $35-70 |
| Centrifugal humidifier | 1-3 L/h | 100-200 W at 230 V | from about $56 |

## Recommendation

**Small apple fridge (up to about 20 m³):** the 4-disc board you have is enough (20 m³ × 8 ml/h = 0.16 L/h). Run it from the 5 V 5 A supply.

**25-30 m² room (75-90 m³): 2-3 single-head 24 V foggers** (0.6-0.9 L/h).
- Put 2 heads on relay channel 1 and 1 head on channel 2, both in one tank in front of the cooler fan.
- Each head comes with its own 24 V adapter. The relay switches the 24 V cable, so you never touch 230 V.
- Keep the ESP32, sensors and relays on the 5 V supply (2 A is plenty without the 5 V discs).
- Tank: a 20-30 L lidded plastic box gives 3-5 days between refills. With a float valve on an RO filter it never needs refilling.

## Power and running cost (75-90 m³ example)

| Part | Power | Running |
|---|---|---|
| 3 × 24 V fogger | 3 × 18 W = 54 W | on about half the time: about 0.65 kWh/day, 20 kWh/month |
| ESP32, sensors, relays | about 1.5 W | about 1 kWh/month |

## Budget (75-90 m³ room, AliExpress app prices Oct 2026, shipping estimated)

| Part | Qty | Cost |
|---|---|---|
| 24 V fogger, 1 head, with adapter | 3 | $31 |
| XY-MD03 RS485 sensors (1 at first-order price) | 4 | $34 |
| TTL to RS485 module | 1 | $0.76 |
| CAT5E cable (dac.ge) | 20 m | $7.40 |
| IP67 box + cable glands | 1 + 5 | $5.44 |
| 20-30 L tank with lid (local) | 1 | about $8 |
| Shipping (about 4 sellers) | | about $24 |
| **Total** | | **about $110** |

You already have the ESP32, 5 V supply and relays. Demineralised water at 3-8 L a day adds up, so a small RO filter (around $30-60) pays for itself within a season.
