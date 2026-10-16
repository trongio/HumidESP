# Looking after HumidESP

A short routine keeps the humidifiers misting and the readings honest. Times assume an apple store at 0-4 °C and 90-95 %RH.

## Every week (5 minutes)

- **Open the web page** and look at the Events list. Repeated "no humidity rise", "long run" or "sensor offline" alerts point to the problems below.
- **Check the water level** and top up with **demineralised or RO water** only. Tap water leaves white mineral dust on the apples and kills ultrasonic discs.
- **Look at the mist.** It should be an even, steady plume. Weak or patchy mist means dirty or worn discs.
- **Compare the sensors.** In a still room they should agree within about 3 %RH. One that drifts away from the others needs cleaning or an offset (Sensors → Edit → Humidity offset).

## Every 2-4 weeks

- **Clean the ultrasonic discs.** Unplug the power, take the discs out, soak them 10 minutes in white vinegar or citric acid (1 spoon per glass of water), rinse in clean water. Never scrape the disc surface.
- **Empty and wipe the water tank.** Cold standing water still grows slime. Rinse with a little vinegar, then clean water.
- **Check the float switch** (if fitted) moves freely.

## Every season (before loading apples)

- **Calibrate the sensors.** Put all of them together in one spot for an hour (a closed box with a damp cloth works) and set offsets so they read the same. For an absolute check use the salt test: a sealed jar with a slurry of table salt and a little water sits at 75 %RH after a few hours.
- **Replace discs** that mist weakly after cleaning. Expect about 1,000-3,000 hours from the small 5 V discs, 3,000-5,000 hours from 24 V industrial fogger discs.
- **Inspect the electronics box:** no water inside, glands tight, no green corrosion on terminals or the relay. Re-tighten screw terminals.
- **Check the sensor cable** for cuts and crushed spots where it passes the door.
- **Test the alarms:** Settings → Send test alert, and make sure it reaches your phone (ntfy).
- **Check the firmware:** System → Check for update.

## Things that destroy hardware

| Cause | What happens | Prevention |
|---|---|---|
| Running dry | an ultrasonic disc cracks within minutes | keep "Stop auto humidifiers if humidity doesn't rise" on, fit a float switch |
| Hard water | discs scale up and lose output, white dust on fruit | demineralised or RO water only |
| Ice | frozen water cracks discs when they start | keep the water tank above 0 °C (or outside the cold zone) |
| Mist on electronics | corrosion, short circuits | mount the atomizer board above and away from the mist, box open at the bottom, conformal coating |
| Mist on sensors | reads 100 % and the humidifiers stop early | keep sensors 1.5 m or more from the plume, use the Auto layout positions |
| Cold blast on sensors | reads too dry, humidifiers overshoot | keep sensors out of the cooler's direct air stream |

## Troubleshooting

| Symptom | Likely cause | Fix |
|---|---|---|
| "No humidity rise" alert, humidifier stopped | empty tank, dead or dirty discs, board not starting | refill or clean, then press Resume. If the board needs a button press after power on, see the Atomizer board check in the README |
| Humidity never reaches the target | too little mist for the room, door opened often, cooler drying the air hard | add discs or switch to a bigger humidifier (see docs/shopping.html), lower the target a little |
| Sensor offline | cable, loose terminal, wrong address | check wiring. RS485: Sensors → RS485 tools → Scan bus |
| Sensor stuck at 99-100 % | condensation or mist on it | move it out of the plume, let it dry. SHT4x probes run a short heater automatically above 95 % |
| ESP resets when misting starts | power supply too small or thin wires | see "Current budget" in the README. Use thick, short wires, or give the ESP its own supply |
| Page unreachable on home Wi-Fi | router changed the IP | try http://humidesp.local, or join the HumidESP hotspot and open http://192.168.4.1 |
| Forgot the web password | | hold the board's BOOT button for 10 s while it runs |

## Spare parts worth keeping

- 2-4 spare ultrasonic discs (the same size as yours)
- one spare sensor of the type you use
- a spare relay module
- 5 L of demineralised water
