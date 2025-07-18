/// Custom data types
struct Timer {
	double time = 0.0;
	double limit = 60.0;

	void start() { time = limit; }
	void reset() { time = 0; }
	void tick() { time -= 1; }
	void tick(double tDT) { time -= tDT; }
	bool isOver() { return time <= 0; }
};
struct SaveData {
	unsigned int points;
	unsigned int prestiege;
	bool upg[upgradeCount];

	void obtain(unsigned int tId) {
		if (tId >= upgradeCount) return;
		obtainedUpgradeCount += 1;
	}
	bool holdToClick() { return upg[0]; }
	bool multiFrog() { return upg[1]; }
	bool idleGain() { return upg[2]; }
	bool betterHoldToClick() { return upg[3]; }
	bool hasAll() {
		for (unsigned int i = 0; i < upgradeCount; i++)
			if (!upg[i]) return false;
		return true;
	}

	void load() {
		if (!std::filesystem::exists("save")) return;
		//Parse save if it exists
		std::vector<std::string> saveFile = StrSplit(StrFromFile("save"), '\n');
		points = std::stoi(saveFile[0]);
		if (saveFile.size() > 1) prestiege = std::stoi(saveFile[1]);
		//Load upgrades
		if (saveFile.size() > 2) {
			upg[0] = std::stoi(saveFile[2]);
			upg[1] = std::stoi(saveFile[3]);
			upg[2] = std::stoi(saveFile[4]);
		}
		if (saveFile.size() > 5) upg[3] = std::stoi(saveFile[5]);
		//Enumerate obtained updates
		for (unsigned int i = 0; i < upgradeCount; i++)
			if (upg[i]) obtainedUpgradeCount += 1;
	}
	void save() {
		StrToFile("save",
			std::to_string(points) + "\n"
			+ std::to_string(prestiege) + "\n"
			+ std::to_string(holdToClick()) + "\n"
			+ std::to_string(multiFrog()) + "\n"
			+ std::to_string(idleGain()) + "\n"
			+ std::to_string(betterHoldToClick()));
	}
};