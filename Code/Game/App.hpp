#pragma once
#include "Engine/Core/EventSystem.hpp"

class Game;
class App;
class Texture;

struct OBJ_Model;
class glTF_Asset;

extern App* g_app;

class App
{
public:
	static bool EventQuit(EventArgs& args);

public:
	App();
	~App();

	void RunMainLoop();
	void RunFrame();
	void Update();
	void Render() const;
	void Startup();

	void Startup_PopulateBlackboard();
	void Startup_LoadAllModels();
	void Startup_LoadAllglTF();
	void Startup_DisplayCommandsToDevConsole();

	void SetIsQuitting();
	void GameReset();
	bool IsQuitting() const;
	bool IsDebug() const;

	bool IsKeyDown(unsigned char keyCode);
	bool WasKeyJustPressed(unsigned char keyCode);
	bool WasKeyJustReleased(unsigned char keyCode);

	Texture* getTextureToDrawTo();

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Models
	OBJ_Model* m_blaineModel;
	OBJ_Model* m_venipedeModel;
	OBJ_Model* m_6sharksModel;
	OBJ_Model* m_theWindModel;

	glTF_Asset* m_dragonModel;

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Textures
	Texture* m_textureToDrawTo;

	Game* m_game = nullptr;

private:
	
	bool m_isQuitting = false;
	bool m_isDebug = false;
	float m_lastFrameTime = 0.f;
	float m_lastFrame = 0.f;

	bool m_isKeyDownArray[256] = { false };
	bool m_wasKeyDownPrevArray[256] = { false };
};