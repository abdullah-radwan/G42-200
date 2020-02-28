#include "..\Header\G422.h"
#include "..\Header\G422_MDL_DVC.h"
#include "..\Header\G422_DVC.h"

void G422::cueEngines(APU& eng, APU::SIMSTATE sst)
{
	if (eng.state == sst) return;

	switch (sst)
	{
	case APU::ENG_START:
		if (eng.state == APU::ENG_RUN) return; // can't set a running engine to start again
		PlayVesselWave(SFXID, SFX_APU_START);

		break;
	case APU::ENG_RUN:
		if (eng.state == APU::ENG_STOP) return; // engine doesn't go just from stop to run
		StopVesselWave(SFXID, SFX_APU_START);
		StopVesselWave(SFXID, SFX_APU_STOP);

		break;
	case APU::ENG_STOP:
		if (eng.state == APU::ENG_STOP) return; // can't stop what's not running
		StopVesselWave(SFXID, SFX_APU_RUN);
		StopVesselWave(SFXID, SFX_APU_START);
		if (eng.state == APU::ENG_RUN) PlayVesselWave(SFXID, SFX_APU_STOP); // only makes sound after it's running (should be set by rpm, really)

		break;
	}

	eng.state = sst;
}

void G422::simEngines(double& dT, APU& eng)
{
	double fuelLvl = GetPropellantMass(fuel_sys);

	if (eng.feed & APU::FUEL_PUMP && (fuelLvl < 0.001 || !(eng.feed & APU::FUEL_OPEN)))
		clbkVCMouseEvent((VC_CTRLSET_SWITCHES << 16) | (VC_swIndexByMGID[MGID_SW2_SYSFEED_APU] & 0xFFFF), PANEL_MOUSE_RBPRESSED, _V0);
	
	if (eng.state == APU::ENG_INOP) return;

	if (eng.feed & APU::FUEL_PUMP) // simulate fuel line pressure from pumps
	{
		if (eng.fuelPrs < 1)
		{
			double deltaPrs = 0.22 * dT;
			eng.fuelPrs = min(eng.fuelPrs + deltaPrs, 1);
		}
	}
	else
	{
		if (eng.fuelPrs > 0)
		{
			double deltaPrs = -0.12 * (1 - eng.pwrPct * .5) * dT;
			eng.fuelPrs = max(eng.fuelPrs + deltaPrs, 0);
		}
	} 

	if (fuelLvl < 0.001) eng.fuelPrs = 0;

	if (eng.state != APU::ENG_STOP && eng.fuelPrs == 0) return cueEngines(eng, APU::ENG_STOP);
	
	switch (eng.state)
	{
	case APU::ENG_RUN:
	{
		eng.fuelFlow = APU_FUEL_RATE;
		eng.exhaustTemp = 527;
		
		if (eng.hydFeed != APU::HYD_OFF)
		{
			if (eng.hydSys->hydFlow < 2.86)
			{
				eng.hydSys->hydFlow = min(eng.hydSys->hydFlow + (2.86 * 0.28 * dT), 2.86);
				eng.hydSys->hydPrs = eng.hydSys->hydFlow * 1050;
			}
		}
		else if (eng.hydSys->hydFlow > 0)
		{
			eng.hydSys->hydFlow = max(eng.hydSys->hydFlow - (2.86 * 0.12 * dT), 0);
			eng.hydSys->hydPrs = eng.hydSys->hydFlow * 1050;
		}

		double usedFuel = APU_FUEL_RATE * dT;
		eng.usedFuel += usedFuel;
		SetPropellantMass(fuel_sys, fuelLvl - usedFuel);

		PlayVesselWave(SFXID, SFX_APU_RUN);

		return;
	}
	case APU::ENG_START:
	{
		eng.pwrPct += 0.28 * dT;
		eng.fuelFlow = APU_FUEL_RATE * eng.pwrPct;
		eng.exhaustTemp = 527 * eng.pwrPct;

		double usedFuel = eng.fuelFlow * dT;
		eng.usedFuel += usedFuel;
		SetPropellantMass(fuel_sys, fuelLvl - usedFuel);

		if (eng.pwrPct >= 1) { eng.pwrPct = 1; cueEngines(eng, APU::ENG_RUN); }
		return;
	}
	case APU::ENG_STOP:
	{
		eng.pwrPct -= 0.12 * dT;
		eng.fuelFlow = 0;
		eng.exhaustTemp = 527 * eng.pwrPct;

		if (eng.hydSys->hydFlow > 0)
		{
			eng.hydSys->hydFlow = min(2.86 * eng.pwrPct, eng.hydSys->hydFlow);
			eng.hydSys->hydPrs = eng.hydSys->hydFlow * 1050;
		}

		if (eng.pwrPct <= 0)
		{
			eng.pwrPct = 0;
			eng.fuelFlow = 0;
			eng.exhaustTemp = 0;
			eng.hydSys->hydFlow = 0;
			eng.hydSys->hydPrs = 0;
			cueEngines(eng, APU::ENG_INOP);
		}
		return;
	}
	}
}

