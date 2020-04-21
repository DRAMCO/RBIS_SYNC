# Masterthesis Thomas Feys: 'A Wi-Fi based synchronisation solution for a DAQ'
## Discription
This repository contains the code for my masterthesis which conserns the synchronisation for a DAQ system. 
The synchronisation is provided by connecting a GPIO pin of an ESP32 to the DAQ system. The ESP32 will
toggle a pin every second. This will cause a pin interrupt on the DAQ system which will start the sampling
window for the DAQ. 

## Structure
There are three different implementations for the synchronisation. These are further explained below.
- [RBIS 1AP](RBIS_1AP)
  * [TM](RBIS_1AP/TM)
  * [TS](RBIS_1AP/TS)
- [RBIS 2AP](RBIS_2AP)
  * [TM1](RBIS_2AP/TM1)
  * [TM2](RBIS_2AP/TM2)
  * [TM3](RBIS_2AP/TM3)
  * [Slave](RBIS_2AP/SLAVE)
- [SKEW COMP](skew_comp)
  * [TM](skew_comp/TM)
  * [TS](skew_comp/TS)



### RBIS 1AP
This repository contains the code for an implementation of RBIS for a single access point. This means
that all the devices that have to be synchronised need to be in range of the same access point. 
There are two different nodes in the system one time master (TM) and one time slave (TS). The time slave
will adjust its timer to match the timer of the time master. 

### RBIS 2AP
This repository contains the code for an implementation of RBIS for two access points. This means
that all the devices that have to be synchronised need to be in range of the two access points. This implementation
can be expanded to work around more than two access points as wel. 
There are 3 different nodes in the system: TM1, TM2 and TM3. TM2 will adjust its timer to match the timer of
TM1 and TM3 will adjust its timer to match the timer of TM2. To cover two access points with the synchronisation,
TM1 is placed in range of access point 1 and TM3 is placed in range of access point 2. To connect the synchronisation
over both AP's TM2 has to be placed in range of both access points. There is also a slave extension that can be used 
to add new nodes to the network in a simple way.

### Skew comp
This repository also contains the code for an implementation of RBIS for a single access point but with the addition 
of skew compensation. This addition should provide a lower synchronisation error. 
