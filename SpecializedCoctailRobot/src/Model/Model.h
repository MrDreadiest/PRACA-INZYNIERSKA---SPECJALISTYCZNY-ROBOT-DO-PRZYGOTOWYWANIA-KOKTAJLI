#ifndef Model_h
#define Model_h

#include <Arduino.h>
#include <Wire.h>

#include "Pinout/Pinout.h"
#include "Configuration/Configuration.h"

#include "JsonCommandParser/JsonCommandParser.h"
#include "DynamicArray/DynamicArray.h"
#include "../lib/AccelStepper/src/AccelStepper.h"
#include "../lib/DS18B20/src/DS18B20.h"

class Relay
{
private:
	int pistonUpPin;
	int pistonDownPin;
	int fanPin;
	int motorPin;

public:
	Relay(int pistonUpPin, int pistonDownPin, int fanPin, int motorPin)
	{
		this->pistonUpPin = pistonUpPin;
		this->pistonDownPin = pistonDownPin;
		this->fanPin = fanPin;
		this->motorPin = motorPin;
		pinMode(pistonUpPin, OUTPUT);
		pinMode(pistonDownPin, OUTPUT);
		pinMode(fanPin, OUTPUT);
		pinMode(motorPin, OUTPUT);
		digitalWrite(pistonUpPin, HIGH);
		digitalWrite(pistonDownPin, HIGH);
		digitalWrite(fanPin, HIGH);
		digitalWrite(motorPin, HIGH);
	}
	void disablePiston()
	{
		digitalWrite(pistonUpPin, HIGH);
		digitalWrite(pistonDownPin, HIGH);
	}
	void pistonUp()
	{
		digitalWrite(pistonUpPin, LOW);
		digitalWrite(pistonDownPin, HIGH);
	}
	void pistonDown()
	{
		digitalWrite(pistonUpPin, HIGH);
		digitalWrite(pistonDownPin, LOW);
	}
	void enableFan()
	{
		digitalWrite(fanPin, LOW);
	}
	void disableFan()
	{
		digitalWrite(fanPin, HIGH);
	}
	void enableMotor()
	{
		digitalWrite(motorPin, LOW);
	}
	void disableMotor()
	{
		digitalWrite(motorPin, HIGH);
	}
};

class LimitSwitch
{
private:
	int limitSwitchPin;

public:
	LimitSwitch(int limitSwitchPin)
	{
		this->limitSwitchPin = limitSwitchPin;
		pinMode(limitSwitchPin, INPUT_PULLUP);
	}
	bool isGantryOver()
	{
		return digitalRead(limitSwitchPin) == LOW ? true : false;
	}
};

class Encoder
{
private:
  int aPin;
  int bPin;
  int counter;

public:
	Encoder(int aPin, int bPin)
	{
		this->aPin = aPin;
		this->bPin = bPin;
		counter = 0;
		pinMode(aPin, INPUT_PULLUP);
		pinMode(bPin, INPUT_PULLUP);
	}
	int getCounter()
	{
		return counter;
	}
	void setCounter(int counter)
	{
		this->counter = counter;
	}
	int getAPin()
	{
		return aPin;
	}
	int getBPin()
	{
		return bPin;
	}
	void handleAttachedInterrupt(){
		//A rising pulse
		digitalRead(getBPin()) == LOW ? setCounter(getCounter() - 1) : setCounter(getCounter() + 1);
		//B rising pulse
		digitalRead(getAPin()) == LOW ? setCounter(getCounter() + 1) : setCounter(getCounter() - 1);
	}
};

class HallSensor
{
private:
  int doutPin;
public:
	HallSensor(int doutPin)
	{
		this->doutPin = doutPin;
		pinMode(doutPin, INPUT_PULLUP);
	}
};

class ThermometerManager
{
private:
	DS18B20 *ds;
	int dqPin;
	uint8_t address[8];

public:
	ThermometerManager(int dqPin)
	{
		this->dqPin = dqPin;
		ds = new DS18B20(dqPin);
		ds->getAddress(this->address);
	}
	float getTemperature()
	{
		return ds->getTempC();
	}
	bool isToHot(){
		return getTemperature() >= MAX_TEMP ? true : false;
	}
};

class DistanceSensor
{
private:
	int trigPin;
	int echoPin;

public:
	DistanceSensor(int trigPin, int echoPin)
	{
		this->trigPin = trigPin;
		this->echoPin = echoPin;
		pinMode(trigPin, OUTPUT);
		pinMode(echoPin, INPUT);
	}
	int getDistance()
	{
		long time;
		digitalWrite(trigPin, HIGH);
		delayMicroseconds(10);
		digitalWrite(trigPin, LOW);
		time = pulseIn(echoPin, HIGH);
		return time / 58;
	}
	bool isVessel(){
		if (getDistance() < MAX_VESSEL_DISTANCE)
		{
			return true;
		}
	}
};

class Model
{
private:
	AccelStepper *stepper;

	Relay *relay;
	Encoder *encoder;
	LimitSwitch *leftLimitSwitch;
	LimitSwitch *rightLimitSwitch;

	ThermometerManager *thermometerManager;
	HallSensor *hallSensor;
	DistanceSensor *distanceSensor;

	DynamicArray *dispenserPositionArray;
	DynamicArray *dispenserSequenceCommand;

	JsonCommandParser *jsonCommandParser;

	String response;
	String lastResponse;
	bool responseFlag;

	bool commandFlag;
	bool canWork;
	int commandSequenceCounter;

	bool calibrationFlag;
	int calibrationCounter;

	bool runAllowedFlag;

	int maxX;
	int minX;

	unsigned long currentTime = 0;
	unsigned long lastTime = 0;
	unsigned long difference = 0;

	unsigned long currentTime_TEMP = 0;
	unsigned long lastTime_TEMP = 0;
	unsigned long timeDifference_TIME = 0;

public:
	Model()
	{
		this->stepper = new AccelStepper(X_INTERFACE_TYPE, SM_STEP, SM_DIR);
		this->stepper->setMaxSpeed(X_MAX_SPEED);
		this->stepper->setAcceleration(X_ACCELERATION);

		this->relay = new Relay(RL_IN3_PIS_UP, RL_IN4_PIS_DOWN, RL_IN1_FAN, RL_IN2_SM);
		this->encoder = new Encoder(EN_A, EN_B);
		this->leftLimitSwitch = new LimitSwitch(AM_LS);
		this->rightLimitSwitch = new LimitSwitch(AM_RS);
		this->thermometerManager = new ThermometerManager(T_DQ);
		this->hallSensor = new HallSensor(AM_HS_D);
		this->distanceSensor = new DistanceSensor(AM_DS_TRIG, AM_DS_ECHO);

		this->dispenserPositionArray = new DynamicArray();
		this->dispenserSequenceCommand = new DynamicArray();
		this->jsonCommandParser = new JsonCommandParser();

		this->responseFlag = false;
		this->response = "";
		this->lastResponse = "";

		this->commandFlag = false;
		this->calibrationFlag = false;
		this->runAllowedFlag = false;
		this->canWork = false;
		this->commandSequenceCounter = 0;
		this->calibrationCounter = 0;

		this->minX = 0;
		this->maxX = 0;
	}

void handleTemperature(){
	currentTime_TEMP = millis();
	timeDifference_TIME = currentTime_TEMP - lastTime_TEMP;

	if (timeDifference_TIME >= 60000UL)
	{
		if(thermometerManager->isToHot()) relay->enableFan();
	}
	if (timeDifference_TIME >= 600000UL)
	{
		if(thermometerManager->isToHot()) relay->disableFan();
	}
}

	void proccess(){
		currentTime_TEMP = millis();
		timeDifference_TIME = currentTime_TEMP - lastTime_TEMP;

		if(calibrationFlag){

			handleTemperature();
			
			if(thermometerManager->isToHot()){
				relay->enableFan();
			}
			else
			{
				relay->disableFan();
			}
			if (commandFlag)
			{
				if (commandSequenceCounter == 0 && distanceSensor->isVessel()) canWork = true;
				if (canWork)
				{
						if (commandSequenceCounter < dispenserSequenceCommand->getN())
						{
							int newDispenserNumber = dispenserSequenceCommand->getArray()[commandSequenceCounter];
							int newPosition = dispenserPositionArray->getArray()[newDispenserNumber];
							goTo(newPosition);
							pour();
							commandSequenceCounter++;
						}
						else
						{
							goHome();
							setResponse("DONE\n");
						}
				}
				else
				{
					if (lastResponse != "NO VESSEL\n")
					{
						setResponse("NO VESSEL\n");
					}
					commandFlag = false;
					canWork = false;
					commandSequenceCounter = 0;
					dispenserSequenceCommand->clearArray();
				}
			}
		}
	}

	void calibration()
	{
		if (!calibrationFlag)
		{
			if (runAllowedFlag)
			{
				relay->enableMotor();
				stepper->run();
			}

			switch (calibrationCounter)
			{
			case 0:
				runAllowedFlag = true;
				calibrationCounter++;
				break;
			case 1:
				if (rightLimitSwitch->isGantryOver())
				{
					stepper->setCurrentPosition(0);
					minX = stepper->currentPosition();
					calibrationCounter++;
				}
				else
				{
					stepper->moveTo(stepper->currentPosition() + 50);
					stepper->runSpeed();
				}
				break;
			case 2:
				if (leftLimitSwitch->isGantryOver())
				{
					maxX = stepper->currentPosition();
					calibrationCounter++;
				}
				else
				{
					stepper->moveTo(stepper->currentPosition() -50);
				}
				break;
			case 3:
				
				if (rightLimitSwitch->isGantryOver())
				{
					stepper->setCurrentPosition(0);
					minX = stepper->currentPosition();
					calibrationCounter++;
				}
				else
				{
					stepper->moveTo(stepper->currentPosition() + 50);
				}
				break;
			case 4:
				calibrationFlag = true;
				runAllowedFlag = false;
				relay->disableMotor();
				calibrationCounter++;
				Serial.print("maxX = ");
				Serial.println(maxX);
				Serial.print("minX = ");
				Serial.println(minX);
				Serial.print("steper pos = ");
				Serial.println(stepper->currentPosition());
				Serial.println("DISPENSERS POSITIONS:");
				dispenserPositionArray->invert();
				break;
			}
		}
	}

	void goHome(){

		Serial.println("home");

		relay->enableMotor();
		stepper->setAcceleration(X_ACCELERATION);
		stepper->moveTo(minX);

		while (stepper->distanceToGo() != 0)
		{
			stepper->run();
		}

		commandSequenceCounter = 0;
		dispenserSequenceCommand->clearArray();

		stepper->setCurrentPosition(0);
		minX = stepper->currentPosition();

		relay->disableMotor();
		commandFlag =false;
		canWork = false;
	}

	void goTo(int position){
	Serial.print("go to:");
	Serial.println(position);

	relay->enableMotor();
	stepper->setAcceleration(X_ACCELERATION);
	stepper->moveTo(position);

	while (stepper->distanceToGo() != 0)
	{
		stepper->run();
	}

	relay->disableMotor();
}

	void pour(){
			delay(500);
			relay->pistonDown();
			delay(2000);
			relay->pistonUp();
			delay(500);
			relay->disablePiston();
		}

	void handleHsRisingPulse()
	{
		if (!calibrationFlag && calibrationCounter == 3)
		{
			dispenserPositionArray->addValue(stepper->currentPosition());
		}
	}

	void handleCommand(String str)
	{
		commandFlag = jsonCommandParser->handleCommand(str, dispenserSequenceCommand);
		if (commandFlag == true)
		{
			setResponse("GOOD COMMAND\n");
		}
		else
		{
			setResponse("BAD COMMAND\n");
		}
	}

	bool isCommandFlag(){
		return this->commandFlag;
	}


//RESPONCE HANDLING
	bool isResponse(){
		return this->responseFlag;
	}

	void setResponse(String response)
	{
		this->responseFlag = true;
		this->response = response;
	}

	String getResponse()
	{
		return this->response;
	}

	void clearResponse(){
		this->lastResponse = this->response;
		this->responseFlag = false;
		this->response = "";
	}

	
	// DistanceSensor *getDistanceSensor(){
	// 	return this->distanceSensor;
	// }

	// HallSensor *getHallSensor()
	// {
	// 	return this->hallSensor;
	// }

	Encoder *getEncoder()
	{
		return this->encoder;
	}

	// LimitSwitch *getLeftLimitSwitch()
	// {
	// 	return this->leftLimitSwitch;
	// }

	// LimitSwitch *getRightLimitSwitch()
	// {
	// 	return this->rightLimitSwitch;
	// }
};


#endif