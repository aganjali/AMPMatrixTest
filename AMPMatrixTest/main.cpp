#pragma once
#include "matrix.h"
#include <iostream>
#include "Helper.h"
#include <windows.h>

using namespace std;
bool Initialze();
bool pick_accelerator();
void RunTest(int from, int to, int step);

Timer hp_timer;

int main()
{
	if (Initialze())
	{
		int startRange = 2, endRange = 1024, mul = 2;
		cout << "This is performance comparator for multipliation of two NxN matrices between CPU and GPU\n";
		
		cout << "\ninsert start value of N: ";
		cin >> startRange;
		cout << "\ninsert end value of N: ";
		cin >> endRange;

		RunTest(startRange, endRange, mul);
	}
	system("PAUSE");
	
	return 0;
}
void RunTest(int from,int to, int step)
{
	int iterations = 2;

	// Timer Warm up
	matrix<float> maTmp (14, 63);
	matrix<float> mbTmp (63, 31);

	maTmp.fill_random(1, 100);
	mbTmp.fill_random(1, 100);

	hp_timer.Start();

	matrix<float> mcTmp_g = maTmp.multiply_tile_amp<16>(&mbTmp);
	matrix<float> mcTmp_c = maTmp.multiply_cpu(&mbTmp);
	
	hp_timer.Stop();
	double t = hp_timer.Elapsed();

	//------
	while (from <= to)
	{
	
		double sumCpuTime = 0, sumGpuTime = 0, Accuracy = 0;
		for (int i = 0; i < iterations; i++)
		{
			matrix<float> ma(from, from), mb(from, from);

			ma.fill_random(1, 100);
			mb.fill_random(1, 100);

			matrix<float> mc_g;

			hp_timer.Start();
			if (from <= 32)
				mc_g = ma.multiply_tile_amp<4>(&mb);
			else if (from <= 512)
				mc_g = ma.multiply_tile_amp<16>(&mb);
			else 
				mc_g = ma.multiply_tile_amp<32>(&mb);

			hp_timer.Stop();
			double gpuTime = hp_timer.Elapsed();
			sumGpuTime += gpuTime;
			hp_timer.Start();
			matrix<float> mc_c = ma.multiply_cpu(&mb);
			hp_timer.Stop();

			double cpuTime = hp_timer.Elapsed();
			sumCpuTime += cpuTime;

			if (mc_c.verify(&mc_g))
				Accuracy += 1;
		

		}
		Accuracy = (Accuracy / iterations) * 100;
		float avgCpuTime = sumCpuTime / iterations;
		float avgGpuTime = sumGpuTime / iterations;

		std::cout <<"Dims="<< from << "x" << from << "x" << from << "\tCPU Time: " << avgCpuTime << " ms\t" << "GPU Time: " << avgGpuTime << " ms\t" << "Acc: " << Accuracy << "%" << endl ;

		from *= step;
	}
}
bool Initialze()
{
	if (pick_accelerator())
	{
		hp_timer.Start();
	
		Sleep(100);
		hp_timer.Stop();
		float t= hp_timer.Elapsed();
		return true;
	}
	else
	{
		std::cout << "Could not pick an accelerator";
		return false;
	}
	
}
bool pick_accelerator()
{
	std::vector<accelerator> accs = accelerator::get_all();
	accelerator chosen_one;

	auto result =
		std::find_if(accs.begin(), accs.end(), [](const accelerator& acc)
	{
		return !acc.is_emulated &&
			acc.supports_double_precision &&
			!acc.has_display;
	});

	if (result != accs.end())
		chosen_one = *(result);

	std::wcout << chosen_one.description << std::endl;

	bool success = accelerator::set_default(chosen_one.device_path);
	return success;
}


