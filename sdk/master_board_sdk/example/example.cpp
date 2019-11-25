#include <assert.h>
#include <unistd.h>
#include <chrono>
#include <math.h>
#include <stdio.h>
#include <sys/stat.h>
#include <deque>
#include <vector>
#include <numeric>
#include <functional>

#include "master_board_sdk/master_board_interface.h"
#include "master_board_sdk/defines.h"

#undef N_SLAVES_CONTROLED
#define N_SLAVES_CONTROLED 2
#define N_SLIDER 6

int main(int argc, char **argv)
{
	int cpt = 0;
	double dt = 0.001;
	double t = 0;
	double kp = 1.;
	double kd = 0.1;
	double iq_sat = 1.0;
	double freq = 0.5;
	double amplitude = 9*M_PI;
	double init_pos[N_SLAVES * 2] = {0};

	double sliders_zero[N_SLIDER];
	double sliders[N_SLIDER];
	double sliders_filt[N_SLIDER];

	std::vector<std::deque<double> > sliders_filt_buffer(N_SLIDER);
    size_t max_filt_dim = 200;
    for (unsigned i = 0; i < sliders_filt_buffer.size(); ++i)
    {
        sliders_filt_buffer[i].clear();
    }

	// Use for debugging.
	FILE * pFile;

	int state = 0;
	nice(-20); //give the process a high priority
	printf("-- Main --\n");
	//assert(argc > 1);
	MasterBoardInterface robot_if(argv[1]);
	robot_if.Init();
	//Initialisation, send the init commands
	for (int i = 0; i < N_SLAVES_CONTROLED; i++)
	{
		robot_if.motor_drivers[i].motor1->SetCurrentReference(0);
		robot_if.motor_drivers[i].motor2->SetCurrentReference(0);
		robot_if.motor_drivers[i].motor1->Enable();
		robot_if.motor_drivers[i].motor2->Enable();
		robot_if.motor_drivers[i].EnablePositionRolloverError();
		robot_if.motor_drivers[i].SetTimeout(5);
		robot_if.motor_drivers[i].Enable();
	}

	// Reverse the direction of the first two motors.
	robot_if.motors[0].SetDirection(true);
	robot_if.motors[1].SetDirection(true);

	std::chrono::time_point<std::chrono::system_clock> started = std::chrono::system_clock::now();
	std::chrono::time_point<std::chrono::system_clock> last = std::chrono::system_clock::now();
	while (1)
	{
		if (((std::chrono::duration<double>)(std::chrono::system_clock::now() - last)).count() > dt)
		{
			last = std::chrono::system_clock::now(); //last+dt would be better
			cpt++;
			t += dt;
			robot_if.ParseSensorData(); // This will read the last incomming packet and update all sensor fields.

			// Parse the slider values.
			for (int k = 0; k < 2; k++)
			{
				for (int j = 0; j < 2; j++)
				{
					sliders[2*k + j] = robot_if.motor_drivers[k].adc[j];
				}
			}

			// Filter the slider values once all motors are enabled.
			if (state > 0) {
				for (unsigned i = 0; i < N_SLIDER; ++i)
				{
					if (!robot_if.motors[i].IsEnabled())
					{
						continue;
					}
					if (sliders_filt_buffer[i].size() >= max_filt_dim)
					{
						sliders_filt_buffer[i].pop_front();
					}
					sliders_filt_buffer[i].push_back(sliders[i]);
					sliders_filt[i] = std::accumulate(sliders_filt_buffer[i].begin(),
													sliders_filt_buffer[i].end(),
													0.0) /
									(double)sliders_filt_buffer[i].size();
				}
			}

			// Use the filtered slideres for multiple legs.
			sliders_filt[2] = 2. * (1. - sliders_filt[1]);
			sliders_filt[3] = sliders_filt[1];
			sliders_filt[4] = -2. * (1. - sliders_filt[1]);
			sliders_filt[5] = -sliders_filt[1];
			sliders_filt[1] = sliders_filt[0];

			switch (state)
			{
			case 0: //check the end of calibration (are the all controlled motor enabled and ready?)
				state = 1;
				for (int i = 0; i < N_SLAVES_CONTROLED * 2; i++)
				{
					if (!(robot_if.motors[i].IsEnabled() && robot_if.motors[i].IsReady()))
					{
						state = 0;
					}
					t = 0;																					//to start sin at 0
				}
				break;
			case 1:
				// Zero the motors once all motors are ready.
				for (int i = 0; i < N_SLAVES_CONTROLED * 2; i++)
				{
					robot_if.motors[i].ZeroPosition(); //initial position
				}

				// Zero the slider values.
				for (int i = 0; i < N_SLIDER; i++) {
					sliders_zero[i] = sliders_filt[i];
				}
				if (sliders_filt_buffer[0].size() == max_filt_dim) {
					pFile = fopen ("sensor_values.txt", "w");
					fprintf (pFile, "time_index;time_duration;sliders[0];sliders[1];silders_filt[0];sliders_filt[1];p_ref;motor_pos;p_err;v_err;cur;\n");
					state = 2;
				}
				break;
			case 2:
				// Write debug logs.
				fprintf (pFile, "%d;%0.5f;%0.4f;%0.4f;%0.4f;%0.4f", cpt,
						((std::chrono::duration<double>)(last - started)).count(),
						sliders[0], sliders[1],
						sliders_filt[0], sliders_filt[3]);

				//closed loop, position
				for (int k = 0; k < N_SLAVES_CONTROLED; k++)
				{
					for (int j = 0; j < 2; j++) {
						int i = k * 2 + j;

						if (robot_if.motors[i].IsEnabled())
						{
							// double ref = amplitude * (2. * (robot_if.motor_drivers[k].adc[j] - 0.5) + sin(2 * M_PI * freq * t));
							// double v_ref = 2. * M_PI * freq * amplitude * cos(2 * M_PI * freq * t);
							double ref = amplitude * (sliders_filt[i] - sliders_zero[i]);
							double v_ref = 0.;
							double p_err = ref - robot_if.motors[i].GetPosition();
							double v_err = v_ref - robot_if.motors[i].GetVelocity();
							double cur = kp * p_err + kd * v_err;
							if (cur > iq_sat)
								cur = iq_sat;
							if (cur < -iq_sat)
								cur = -iq_sat;
							robot_if.motors[i].SetCurrentReference(cur);
							fprintf (pFile, ";%0.4f;%0.4f;%0.4f;%0.4f;%0.4f", ref, robot_if.motors[i].GetPosition(), p_err, v_err, cur);
						}
					}
				}
				fprintf (pFile, "\n");
				break;
			}
			if (cpt % 100 == 0)
			{
				printf("\33[H\33[2J"); //clear screen
				robot_if.PrintIMU();
				robot_if.PrintADC();
				robot_if.PrintMotors();
				robot_if.PrintMotorDrivers();
				fflush(stdout);
			}
			robot_if.SendCommand(); //This will send the command packet
		}
		else
		{
			std::this_thread::yield();
		}
	}
	fflush(pFile);
	fclose(pFile);
	return 0;
}
