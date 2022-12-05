# Letterman
This project's aim is to build a battery powered notification for mailboxes. We use 2 LoRa2 dev boards. One is the sensor node in the mailbox and sends notifications via LoRaWAN. The other one is used as a gateway to publish the received messages from LoRaWAN to an mqtt broker on wifi. 

The LoRa device in the mailbox uses a reed switch to recognize when the mailbox door is opened and a motion sensor to detect when you mail has been put inside the box. It is powered by a battery and is optimized for minumum power consumption. 


# Required Hardware
 * 2x TTGO LoRa32 v2.1.6 Dev board: <https://amzn.to/3Bz6E9h>
 * 1x Reed Switch: <https://amzn.to/3UzvxZ5>
 * 1x SR501 PIR motion sensor: <https://amzn.to/3iG0Q6Y>