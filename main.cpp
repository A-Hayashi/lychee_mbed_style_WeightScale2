/* Derived from Adafruit RGB_matrix_Panel library */

#include "P3RGB64x32MatrixPanel.h"
#include "PS_PAD.h"
#include "Weight.h"
#include "ThingSpeak.h"
#include "mbed.h"

typedef struct {
	uint8_t stable;
	float weight;
} weight_mail_t;

typedef struct {
	int analog_rx;
	int analog_ry;
	int analog_lx;
	int analog_ly;
	int buttons;
} pspad_mail_t;

typedef struct{
	uint8_t message_type;
	union{
		weight_mail_t weight;
		pspad_mail_t pspad;
	};
} mail_t;

Mail<mail_t, 32> mail_box;

//P5_14 D0	R1
//P5_8 	D3	G1
//P5_15 D1	B1
//P5_9 	D2	R2
//P5_11 D4	G2
//P5_10 D5 	B2
//P3_10 D8	CLK
//P2_1	D6	D
//P4_0  	OE
//P1_7 	D14	A
//P2_0	D7	B
//P6_13 D10	C
//P4_1		LAT

void draw_main();
void pspad_main();
uint16_t Wheel(P3RGB64x32MatrixPanel &matrix, byte WheelPos);

Thread draw_main_task(osPriorityNormal, 500 * 1024);
Thread pspad_main_task(osPriorityAboveNormal, 100 * 1024);

Serial pc(USBTX, USBRX);

int main() {
	draw_main_task.start(&draw_main);
	pspad_main_task.start(&pspad_main);
	while (true);
	return 0;
}

void pspad_main(){
	pc.printf("pspad main start\n");
	PS_PAD pad(P6_14, P6_15, P6_12, P3_9);
	pad.init();
	int analog_rx;
	int analog_ry;
	int analog_lx;
	int analog_ly;
	int buttons;

	Thread::wait(1000);
	while(true){
		pad.poll();
		analog_rx = pad.read(PS_PAD::ANALOG_RX);
		analog_ry = pad.read(PS_PAD::ANALOG_RY);
		analog_lx = pad.read(PS_PAD::ANALOG_LX);
		analog_ly = pad.read(PS_PAD::ANALOG_LY);
		buttons   = pad.read(PS_PAD::BUTTONS);

		if(buttons){
			mail_t *mail = mail_box.alloc();
			mail->message_type = 2;
			mail->pspad.analog_rx = analog_rx;
			mail->pspad.analog_ry = analog_ry;
			mail->pspad.analog_lx = analog_lx;
			mail->pspad.analog_ly = analog_ly;
			mail->pspad.buttons = buttons;

			mail_box.put(mail);
		}
		Thread::wait(100);
	}
}

void draw_main() {
	pc.printf("draw main start\n");
	ThingSpeak_init();
	weight_init();
	P3RGB64x32MatrixPanel matrix(D0, D3, D1, D2, D4, D5, D8, D6, P4_0, D14, D7,	D10, P4_1);
	matrix.begin();

	while (true) {
		osEvent evt = mail_box.get();
		if (evt.status == osEventMail) {
			mail_t *mail = (mail_t *) evt.value.p;
			if(mail->message_type==1){
				pc.printf("stable:%d\t weight:%f\n", mail->weight.stable, mail->weight.weight);

				matrix.fillScreen(matrix.color444(0, 0, 0));
				matrix.setTextCursor(0, 0);

				matrix.setTextColor(matrix.color444(15, 0, 0));
				matrix.printf("%1d\n%5.1f kg\n",  mail->weight.stable, mail->weight.weight);
			}else if(mail->message_type==2){
				pc.printf("%4d\t%4d\t%4d\t%4d\t%04x\n", mail->pspad.analog_rx, mail->pspad.analog_ry, mail->pspad.analog_lx, mail->pspad.analog_ly, mail->pspad.buttons);

				matrix.fillScreen(matrix.color444(0, 0, 0));
				matrix.setTextCursor(0, 0);

				matrix.setTextColor(matrix.color444(15, 0, 0));
				matrix.printf("%4d %4d\n", mail->pspad.analog_rx, mail->pspad.analog_ry);

				matrix.setTextColor(matrix.color444(0, 15, 0));
				matrix.printf("%4d %4d\n", mail->pspad.analog_lx, mail->pspad.analog_ly);

				matrix.setTextColor(matrix.color444(0, 0, 15));
				matrix.printf("%04x", mail->pspad.buttons);

				if(mail->pspad.analog_ry>100){
					ThingSpeak_ChannelUpdate(mail->pspad.analog_ry, mail->pspad.analog_ly);
				}
			}
			mail_box.free(mail);
		}
	}
}

void weight_result(uint8_t stable, float weight) {
	mail_t *mail = mail_box.alloc();
	mail->message_type = 1;
	mail->weight.stable = stable;
	mail->weight.weight = weight;
	mail_box.put(mail);
}
