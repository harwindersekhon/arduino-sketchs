#include <Arduino.h>
#include <IRremote.hpp> // Ensure you have installed the IRremote library

const int RECV_PIN = 2; // The digital pin where the IR receiver is connected

void setup() {
  Serial.begin(9600);
  
  // Start the receiver
  IrReceiver.begin(RECV_PIN, ENABLE_LED_FEEDBACK); 
  Serial.println("Ready to receive IR signals...");
}

void loop() {
  if (IrReceiver.decode()) {
    /* * Print a summary of the received data 
     * This includes the Protocol (NEC, Sony, etc.) and the Hex code
     */
    Serial.println("Signal Received:");
    IrReceiver.printIRResultShort(&Serial);
    
    // Print the specific hex value we usually need for 'if' statements
    Serial.print("Hex Value: ");
    Serial.println(IrReceiver.decodedIRData.decodedRawData, HEX);
    
    Serial.println("------------------------------------");
    
    // Receive the next value
    IrReceiver.resume(); 
  }
}