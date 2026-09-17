#include <Arduino.h>

#define DBG(x) Serial.println(x)

class Display3Digit
{
public:
	Display3Digit(const byte(&seg)[7], const byte(&dig)[3], byte dp) : pinDp(dp)
	{
		for (byte i = 0; i < 7; i++)			pinSegment[i] = seg[i];
		for (byte i = 0; i < 3; i++)			pinDigit[i] = dig[i];
	}

	void begin()
	{
		for (byte i = 0; i < 7; i++)
		{
			pinMode(pinSegment[i], OUTPUT);
			digitalWrite(pinSegment[i], LOW);
		}

		pinMode(pinDp, OUTPUT);
		digitalWrite(pinDp, LOW);

		for (byte i = 0; i < 3; i++)
		{
			pinMode(pinDigit[i], OUTPUT);
			digitalWrite(pinDigit[i], LOW);
		}
	}

	void update()
	{
		if (micros() - lastRefresh < refreshRateMicro)	return;
		lastRefresh += refreshRateMicro;

		disableDigits();

		byte pattern = font[10]; // leer

		byte n = values[currentDigit];

		if (n <= 9)			pattern = font[n];
		else if (n == 11)	pattern = font[11];

		if (dpState[currentDigit])	pattern |= 0b10000000;

		writeSegments(pattern);

		digitalWrite(pinDigit[currentDigit], HIGH);

		currentDigit++;

		if (currentDigit >= 3)	currentDigit = 0;
	}

	void show(int number)
	{
		if (number > 999)	number = 999;
		if (number < -99)	number = -99;

		if (number < 0)
		{
			number = -number;

			values[0] = 11; // minus
			values[1] = number / 10;
			values[2] = number % 10;

			if (number < 10) { values[0] = 10; values[1] = 11; }	//minus an 2te stelle
		}
		else
		{
			values[0] = number / 100;
			values[1] = (number / 10) % 10;
			values[2] = number % 10;

			if (number < 100)	values[0] = 10;
			if (number < 10)	values[1] = 10;
		}
	}

	void setDP(byte pos, bool state) { if (pos < 3) dpState[pos] = state; }

private:
	byte pinSegment[7];
	byte pinDigit[3];
	byte pinDp;

	byte values[3] = {10, 10, 10};

	bool dpState[3] = {false, false, false};

	byte currentDigit = 0;

	uint32_t refreshRateMicro = 1000;	// 1000 = > 1kHz
	uint32_t lastRefresh = 0;

	// 0b DP,G,F,E,D,C,B,A
	const byte font[12] =
	{
		0b00111111, // 0
		0b00000110, // 1
		0b01011011, // 2
		0b01001111, // 3
		0b01100110, // 4
		0b01101101, // 5
		0b01111101, // 6
		0b00000111, // 7
		0b01111111, // 8
		0b01101111, // 9
		0b00000000, // leer
		0b01000000	// minus
	};

	inline void disableDigits()
	{
		digitalWrite(pinDigit[0], LOW);
		digitalWrite(pinDigit[1], LOW);
		digitalWrite(pinDigit[2], LOW);
	}

	inline void writeSegments(byte pattern)
	{
		for (byte i = 0; i < 7; i++)
			digitalWrite(pinSegment[i], pattern & (1 << i));
		digitalWrite(pinDp, pattern & 0b10000000);
	}
};

// ----------------------------

const byte pinSeg[7] = {21, 20, 19, 18, 17, 16, 15};
const byte pinDig[3] = {9, 8, 7};
const byte pinDp = 14;

Display3Digit display(pinSeg, pinDig, pinDp);

void setup()
{
	Serial.begin(115200);
	Serial1.begin(115200);
	DBG("Display gestartet");

	display.begin();
}

// int value = -99;
// uint32_t lastUpdate = 0;

constexpr uint8_t START_BYTE = 0xAA;
int dbValue = 123;

void receiveDB()
{
	static uint8_t state = 0;
	static uint8_t lowByte = 0;
	static uint8_t highByte = 0;

	while (Serial1.available())
	{
		uint8_t data = Serial1.read();

		switch (state)
		{
			// Auf Startbyte warten
		case 0:	if (data == START_BYTE) { state = 1; }		break;

			// Low-Byte
		case 1:
			lowByte = data;
			state = 2;
			break;

			// High-Byte
		case 2:
			highByte = data;
			state = 3;
			break;

			// Prüfsumme
		case 3:
			if (data == (lowByte ^ highByte))
			{
				int16_t value = (int16_t)((uint16_t)lowByte | ((uint16_t)highByte << 8));
				dbValue = value;
			}
			// Egal ob gültig oder nicht: wieder auf Startbyte warten
			state = 0;
			break;
		}
	}
}

void loop()
{

	// dbValue an deine 7-Segment-Anzeige übergeben

	//// 7segment stuff

	// if (millis() - lastUpdate >= 25)
	// {
		// 	lastUpdate = millis();

		// 	if (value < 1000) value++;
		// }
		// display.show(value);

	receiveDB();

	// hier taster rein...wenn (dbValue == 0) => disableDigits() sollte passen... 

	DBG(dbValue);

	display.show(dbValue);

	display.update();

	// display.show(-99 + (millis() / (1000)));

}