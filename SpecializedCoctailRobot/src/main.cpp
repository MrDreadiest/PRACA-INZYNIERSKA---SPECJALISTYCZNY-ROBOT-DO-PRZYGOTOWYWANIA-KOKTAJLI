#include <Arduino.h>
#include <Wire.h>
#include <stdio.h>
#include <SoftwareSerial.h>
#include "../lib/AccelStepper/src/AccelStepper.h"

#include "Pinout/Pinout.h"
#include "Model/Model.h"

SoftwareSerial Bluetooth(BLE_RX, BLE_TX); // RX,TX
Model *model = nullptr;


void enA_RisingPulse();
void enB_RisingPulse();
void hsD_RisingPulse();

void BluetoothHandleCommand();
void BluetoothHandleResponse();
void Proccess();
void Calibration();

void setup() {
    // put your setup code here, to run once:
    Serial.begin(9600);
    Bluetooth.begin(9600);

    attachInterrupt(digitalPinToInterrupt(EN_A), enA_RisingPulse, RISING);
    attachInterrupt(digitalPinToInterrupt(EN_B), enB_RisingPulse, RISING);
    attachInterrupt(digitalPinToInterrupt(AM_HS_D), hsD_RisingPulse, RISING);
    
    while (!Serial || !Bluetooth){}
    model == nullptr ? model = new Model() : model = model;
    
}

void loop() {

    Calibration();

    BluetoothHandleCommand();

    BluetoothHandleResponse();

    Proccess();
}

void enA_RisingPulse(){
    model->getEncoder()->handleAttachedInterrupt();
}
void enB_RisingPulse(){
    model->getEncoder()->handleAttachedInterrupt();
}
void hsD_RisingPulse(){
    model->handleHsRisingPulse();
}

void BluetoothHandleCommand(){
    while (Bluetooth.available() > 0 && !model->isCommandFlag())
    {
        String command = Bluetooth.readString();
        model->handleCommand(command);

        Serial.print(command);
        Bluetooth.print(command);
    }
}

void BluetoothHandleResponse()
{
    while (model->isResponse())
    {
        Bluetooth.print(model->getResponse());
        model->clearResponse();
    }
}

void Calibration(){
    model->calibration();
}

void Proccess()
{
    model->proccess();
}