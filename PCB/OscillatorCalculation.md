# Oscillator Calculations
#### Derived from [ecsxtal.com](https://ecsxtal.com/oscillator-circuit-design)

according to [the crystal datasheet](https://www.jauch.com/downloadfile/5d5283eb8ea55300421fd84937fcba163/jauch_datasheet_j49smh.pdf), 
for our crystal (J49SMH-P-G-G-K-25M0)
we need to have a circuit capacitance of 30 pF

According to load capacitance in ecsxtal:

16 pF = (c1 * c2) / (c2 + c1) + 4 pF


Where we get 3pF from shunt capacitance, the max allowed value by our crystal is 5 pF, usually 3-5 pF standard.

for c1 and c2 you generally want c1 to be 1/10 of c2 according to ecsxtal.

If calculated with c1 = c2/10:
- c2 = 297
- c1 = 29.7

#### CLoad = 30 MHz

The problem is we can't buy these values of capacitors. 
Thus we need to set a value that we can purchase

If we set c1 = 14 pF
- c1 = 30
- c2 = 300

#### CLoad = 344/11 ~= 30.27272727 

This should definitely be within bounds of capacitance load

We can buy 30 pF, and we can buy 300pF
