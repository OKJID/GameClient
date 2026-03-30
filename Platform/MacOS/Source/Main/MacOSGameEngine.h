#pragma once

#include "Common/GameEngine.h"

class MacOSGameEngine : public GameEngine
{
public:
	MacOSGameEngine();
	~MacOSGameEngine() override;

	void init() override;
	void reset() override;
	void update() override;
	void serviceWindowsOS() override;

protected:
	GameLogic* createGameLogic() override;
	GameClient* createGameClient() override;
	ModuleFactory* createModuleFactory() override;
	ThingFactory* createThingFactory() override;
	FunctionLexicon* createFunctionLexicon() override;
	LocalFileSystem* createLocalFileSystem() override;
	ArchiveFileSystem* createArchiveFileSystem() override;
	NetworkInterface* createNetwork() override;
	Radar* createRadar() override;
	WebBrowser* createWebBrowser() override;
	AudioManager* createAudioManager() override;
	ParticleSystemManager* createParticleSystemManager(Bool dummy) override;
};
