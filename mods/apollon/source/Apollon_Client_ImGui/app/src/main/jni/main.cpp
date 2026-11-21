#include <jni.h>
#include <android/native_activity.h>
#include <android/log.h>
#include <errno.h>

#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <sstream>
#include <vector>
#include <unordered_set>
#include <random>
#include <algorithm>
#include <ghc/filesystem.hpp>
namespace fs = ghc::filesystem;
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include <signal.h>
#include <pthread.h>
#include <codecvt>
#include <chrono>
#include <queue>

#include "ImGui/imgui_internal.h"
#include "ImGui/imgui.h"
#include "ImGui/backends/imgui_impl_android.h"
#include "ImGui/backends/imgui_impl_opengl3.h"

#include <EGL/egl.h>
#include <GLES3/gl3.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

#include <sys/system_properties.h>

#include <string>
#include <cmath>
#include <android/keycodes.h>

//====================================
#include "ImGui/FONTS/DEFAULT.h"
//=====================================
#include <Includes/Utils.h>
#include <Includes/Dobby/dobby.h>
#if defined(__aarch64__)
#include "Arm64/And64InlineHook.hpp"
#else
#include "Substrate/SubstrateHook.h"
#endif
#include "KittyMemory/MemoryPatch.h"
#include "KittyScanner/KittyScanner.hpp"
#include "ENCRYPT/obfuscate.h"
#include "ENCRYPT/oxorany.h"
#include "RGB.h"
#include "Chams.h"
#include "Includes/nlohmann/json.hpp"
#include "Includes/json.h"

typedef uint32_t _DWORD;
typedef uint64_t _QWORD;
#define __int8 char
#define __int16 short
#define __int32 int
#define __int64 long long
#define _BYTE unsigned char


using json = nlohmann::json;
int screenWidth = 0;
int screenHeight = 0;
float scaleGlobal, calcResX;
bool g_Initialized = false;
ImGuiWindow* g_window = NULL;
string GameName = oxorany("com.mojang.minecraftpe");
const char* MineLib = OBFUSCATE("libminecraftpe.so");


struct My_Patches {MemoryPatch
Font0, Font1, Font2, Font3, Font4, Font5, Font6, Font7, Font8, Font9, Font10,
NoHurtCam, NoCamDistortion, NoBoatRotation, NoCamSleep, PlaceCamera,
NoSlowDown,
LavaDrown, WaterDrown,
SlowDownTriggers,
Noclip,
AutoClickMine,
XrayCameraThird,
NoEmoteUseCooldown,
NoBlur1, NoBlur2,
FullBright,
AntiKnockBack,
NativeKeyHandler,
NoCaveVignette
;} hexPatches;


#include "offsets.h"
#include "dark_theme.h"
#include "Vector.h"
#include "hook_classes.h"
#include "variables.h"
#include "hook_utils.h"
#include "menu_utils.h"


extern "C" {
    JNIEXPORT void JNICALL Java_com_mojang_minecraftpe_platforms_Platform2_init(JNIEnv* env, jclass cls);
    JNIEXPORT void JNICALL Java_com_mojang_minecraftpe_platforms_Platform2_resize(JNIEnv* env, jobject obj, jint width, jint height);
    JNIEXPORT void JNICALL Java_com_mojang_minecraftpe_platforms_Platform2_step(JNIEnv* env, jobject obj);
    JNIEXPORT void JNICALL Java_com_mojang_minecraftpe_platforms_Platform2_imgui_Shutdown(JNIEnv* env, jobject obj);
    JNIEXPORT void JNICALL Java_com_mojang_minecraftpe_platforms_Platform2_MotionEventClick(JNIEnv* env, jobject obj,jboolean down,jfloat PosX,jfloat PosY);
    JNIEXPORT jstring JNICALL Java_com_mojang_minecraftpe_platforms_Platform2_getWindowRect(JNIEnv *env, jobject thiz);
};

JNIEXPORT void JNICALL
Java_com_mojang_minecraftpe_platforms_Platform2_init(JNIEnv* env, jclass cls) {

    //SetUpImGuiContext
    if(!g_Initialized) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
	
    //Set ImGui Style
    ImGui::StyleColorsClassic();
	embraceTheDarkness();
 
    // Setup Platform/Renderer backends
    ImGui_ImplOpenGL3_Init(oxorany("#version 100"));

    ImFontConfig CustomFont;
    CustomFont.FontDataOwnedByAtlas = false;

	static const ImWchar ranges[] = {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x0100, 0x024F, // Extended Latin
        0x0370, 0x03FF, // Greek
        0x0400, 0x04FF, // Cyrillic
        0x0500, 0x052F, // Cyrillic Supplement
        0x2000, 0x206F, // General Punctuation
        0x2070, 0x209F, // Superscripts and Subscripts
        0x20A0, 0x20CF, // Currency Symbols
        0x2100, 0x214F, // Letterlike Symbols
        0x2150, 0x218F, // Number Forms
        0x2190, 0x21FF, // Arrows
        0x2200, 0x22FF, // Mathematical Operators
        0x2300, 0x23FF, // Miscellaneous Technical
        0x2400, 0x243F, // Control Pictures
        0x2440, 0x245F, // Optical Character Recognition
        0x2460, 0x24FF, // Enclosed Alphanumerics
        0x2500, 0x257F, // Box Drawing
        0x2580, 0x259F, // Block Elements
        0x25A0, 0x25FF, // Geometric Shapes
        0x2600, 0x26FF, // Miscellaneous Symbols
        0x2700, 0x27BF, // Dingbats
        0x3000, 0x303F, // CJK Symbols and Punctuation
        0x3040, 0x309F, // Hiragana
        0x30A0, 0x30FF, // Katakana
        0x4E00, 0x9FFF, // CJK Unified Ideographs
        0xFF00, 0xFFEF, // Halfwidth and Fullwidth Forms
        0,
    };

    io.Fonts->AddFontFromMemoryTTF(const_cast<std::uint8_t*>(Custom3), sizeof(Custom3), 30.f, &CustomFont, ranges);

	static bool loadedCfg = false;
     if (!loadedCfg){
         std::ifstream file(oxorany("/data/data/") + GameName + oxorany("/xal/config.json"));
         if (file.good()){
             loadConfig(oxorany("/data/data/") + GameName + oxorany("/xal/config.json"));
         }
         loadedCfg = true;
     }
	g_Initialized = true;
	}
    return;
}

JNIEXPORT void JNICALL
Java_com_mojang_minecraftpe_platforms_Platform2_resize(JNIEnv* env, jobject obj, jint width, jint height) {
	if (g_Initialized) {
    screenWidth = (int) width;
    screenHeight = (int) height;
    ImGuiIO &io = ImGui::GetIO();
    io.IniFilename = NULL;
    io.DisplaySize = ImVec2((float)screenWidth, (float)screenHeight);
	static bool isLoaded = false;
	if (!isLoaded) {
		ImGuiStyle& style = ImGui::GetStyle();
	    scaleGlobal = calculateResolutionScale(screenWidth, screenHeight, 2480, 1116);
		calcResX = calculateResolutionX(screenWidth, 2480);
	    style.ScaleAllSizes(scaleGlobal * 3.5);
        io.FontGlobalScale = scaleGlobal;
		style.WindowBorderSize                  = scaleGlobal * 2;
        style.ChildBorderSize                   = scaleGlobal * 2;
        style.PopupBorderSize                   = scaleGlobal * 2;
        style.FrameBorderSize                   = scaleGlobal * 2;
        style.TabBorderSize                     = scaleGlobal * 2;
		isLoaded = true;
	}
	}
	return;
}

void Patches() {
    if (inMenu) {
        hexPatches.NoHurtCam.Restore();
        hexPatches.NoCamDistortion.Restore();
        hexPatches.NoBoatRotation.Restore();
        hexPatches.NoCamSleep.Restore();
        hexPatches.PlaceCamera.Restore();
        hexPatches.SlowDownTriggers.Restore();
        hexPatches.XrayCameraThird.Restore();
    } else {
        if (noHurtCam && !panic) {
            hexPatches.NoHurtCam.Modify();
        } else {
            hexPatches.NoHurtCam.Restore();
        }
        if (noCamDist && !panic) {
            hexPatches.NoCamDistortion.Modify();
        } else {
            hexPatches.NoCamDistortion.Restore();
        }
        if (noBoatRotation && !panic) {
            hexPatches.NoBoatRotation.Modify();
        } else {
            hexPatches.NoBoatRotation.Restore();
        }
        if (noCamSleep && !panic) {
            hexPatches.NoCamSleep.Modify();
        } else {
            hexPatches.NoCamSleep.Restore();
        }
        if (placeCam && !panic) {
            hexPatches.PlaceCamera.Modify();
        } else {
            hexPatches.PlaceCamera.Restore();
        }
        if (offSlowdownTrigger && !isLANGame && !panic) {
            hexPatches.SlowDownTriggers.Modify();
        } else {
            hexPatches.SlowDownTriggers.Restore();
        }
        if (xrayCam && !panic) {
            hexPatches.XrayCameraThird.Modify();
        } else {
            hexPatches.XrayCameraThird.Restore();
        }
		if (antiKnockback && !isLANGame && !panic) {
			hexPatches.AntiKnockBack.Modify();
		} else {
			hexPatches.AntiKnockBack.Restore();
		}
	}
    
    if (fontSize == 0 && !panic) {
        hexPatches.Font0.Modify();
    }
    if (fontSize == 1 && !panic) {
        hexPatches.Font1.Modify();
    }
    if (fontSize == 2 && !panic) {
        hexPatches.Font2.Modify();
    }
    if (fontSize == 3 && !panic) {
        hexPatches.Font3.Modify();
    }
    if (fontSize == 4 && !panic) {
        hexPatches.Font4.Modify();
    }
    if (fontSize == 5 && !panic) {
        hexPatches.Font5.Modify();
    }
    if (fontSize == 6 && !panic) {
        hexPatches.Font6.Modify();
    }
    if (fontSize == 7 && !panic) {
        hexPatches.Font7.Modify();
    }
    if (fontSize == 8 && !panic) {
        hexPatches.Font8.Modify();
    }
    if (fontSize == 9 && !panic) {
        hexPatches.Font9.Modify();
    }
    if (fontSize == 10 && !panic) {
        hexPatches.Font10.Modify();
    }
    
    if (noSlowDown && !isLANGame && !panic) {
        hexPatches.NoSlowDown.Modify();
    } else {
        hexPatches.NoSlowDown.Restore();
    }
    
    if (offDrown && !panic) {
        hexPatches.LavaDrown.Modify();
        hexPatches.WaterDrown.Modify();
    } else {
        hexPatches.LavaDrown.Restore();
        hexPatches.WaterDrown.Restore();
    }
    
    if (noclip && !isLANGame && !panic) {
        hexPatches.Noclip.Modify();
    } else {
        hexPatches.Noclip.Restore();
    }
    
    if (clickAutoMine && !panic) {
        hexPatches.AutoClickMine.Modify();
    } else {
        hexPatches.AutoClickMine.Restore();
    }
	
	if (noEmoteCooldown && !panic) {
		hexPatches.NoEmoteUseCooldown.Modify();
	} else {
		hexPatches.NoEmoteUseCooldown.Restore();
	}
	
	if (noBlur && !panic) {
		hexPatches.NoBlur1.Modify();
		hexPatches.NoBlur2.Modify();
	} else {
		hexPatches.NoBlur1.Restore();
		hexPatches.NoBlur2.Restore();
	}
	if (!panic) {
		if (xrayDefault || fullBright) {
			hexPatches.FullBright.Modify();
	    } else {
		    hexPatches.FullBright.Restore();
		}
	} else hexPatches.FullBright.Restore();
	
	if (noCaveVignette && !panic) {
		hexPatches.NoCaveVignette.Modify();
	} else {
		hexPatches.NoCaveVignette.Restore();
	}
}

#include "menu.h"

JNIEXPORT void JNICALL
Java_com_mojang_minecraftpe_platforms_Platform2_step(JNIEnv* env, jobject obj) {
    if (g_Initialized) {
        removeDirectory(oxorany("/data/data/") + GameName + oxorany("/app_lib"));
        saveConfig(oxorany("/data/data/") + GameName + oxorany("/xal/config.json"));
        ImGuiIO& io = ImGui::GetIO();
        
        //Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui::NewFrame();
        
        BeginDraw();
        
        RenderNotifications();
        
        ImGui::EndFrame();
        
        ImGui::Render();
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
}

JNIEXPORT void JNICALL Java_com_mojang_minecraftpe_platforms_Platform2_imgui_Shutdown(JNIEnv* env, jobject obj){
    if (!g_Initialized)
        return;
     // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplAndroid_Shutdown();
    ImGui::DestroyContext();
    g_Initialized=false;
}

JNIEXPORT void JNICALL Java_com_mojang_minecraftpe_platforms_Platform2_MotionEventClick(JNIEnv* env, jobject obj,jboolean down,jfloat PosX,jfloat PosY){
	if (g_Initialized) {
	ImGuiIO & io = ImGui::GetIO();
	io.MousePos = ImVec2(PosX, PosY);
	io.MouseDown[0] = down;
	}
}

JNIEXPORT jstring JNICALL Java_com_mojang_minecraftpe_platforms_Platform2_getWindowRect(JNIEnv *env, jobject thiz) {
    //get drawing window
    // TODO: accomplish getWindowSizePos()
	if (g_Initialized) {
    char result[512]="0|0|0|0";
	if (!panic) {
		if(g_window){
            sprintf(result,"%d|%d|%d|%d",(int)g_window->Pos.x,(int)g_window->Pos.y,(int)g_window->Size.x,(int)g_window->Size.y);
        }
	}
    return env->NewStringUTF(result);
	}
}

void *(*old_Intersects)(Mob *thiz, vec3_t *a2, vec3_t *a3);
void *Intersects(Mob *thiz, vec3_t *a2, vec3_t *a3) {
	if (panic) return old_Intersects(thiz, a2, a3);
	if (inMenu) return old_Intersects(thiz, a2, a3);
	if (thiz == localPlayer) {
		
		if (look360) {
			if (notSaved) {
				saveAngle = *thiz->getRotation();
				saveYHeadRot = thiz->getYHeadRot();
				notSaved = false;
		    }
			thiz->setRotationX(saveAngle.x);
			thiz->setRotationY(saveAngle.y);
			thiz->setYHeadRot(saveYHeadRot);
		} else {
			notSaved = true;
		}
		
		if (!isLANGame) {
		    if (derp) {
				thiz->setYHeadRot(derpGradys);
			    thiz->setYBodyRotation(derpGradys);
		    }
		}
		
		if (fireballRotation || itemRotation || !mobList.empty() || !playerList.empty()) {
			if (rotations) {
                if (!isLANGame && !emulateNativeAttack) thiz->setYBodyRotation(auraAngle.y);
			    thiz->setYHeadRot(auraAngle.y);
                thiz->setRotationX(auraAngle.x);
            }
            if (targetLock) {
                thiz->setRotationY(auraAngle.y);
            }
		}
    }
    return old_Intersects(thiz, a2, a3);
}

void *(*old_NormalTick)(LocalPlayer *thiz);
void *NormalTick(LocalPlayer *thiz) {
	if (panic) return old_NormalTick(thiz);
	localPlayer = NULL;
	if (inMenu) {
		isLoaded = true;
		return old_NormalTick(thiz);
	}
	
	//inMemoryFunction = format("Instance: %llX", thiz);
	
	if (isLoaded) {
		thiz->displayClientMessage(oxorany("§l§0[§5Apollon Client§0]§r Good Luck, Have Fun!"), "");
		isLoaded = false;
	}
	
	localPlayer = thiz;
	vec2_t *playerRot = thiz->getRotation();
	vec3_t *playerPos = thiz->getPosition();
	globalPlayerRot = *playerRot;
	globalPlayerPos = *playerPos;
	Level *myLevel = thiz->getLevel();
	GameMode *myGameMode = thiz->getGameMode();
	BlockSource *myBlockSource = thiz->getDimensionBlockSource();

	if (myBlockSource && xrayDefault) {
    for (int i = renderOresESP.size() - 1; i >= 0; i--) {
        vec3_t orePosition = {(float)renderOresESP[i].orePos.x, (float)renderOresESP[i].orePos.y, (float)renderOresESP[i].orePos.z};
        vec3_t camPos = {CameraPos.x, CameraPos.y, CameraPos.z};
        float distanceToBlock = distanceTo(&camPos, &orePosition);
        
        if (distanceToBlock > 32.0f) {
            renderOresESP.erase(renderOresESP.begin() + i);
            removedCount++;
            continue;
        }
        
        // ПРОВЕРКА НА AIR БЛОК
        Block *currentBlock = myBlockSource->getBlock(renderOresESP[i].orePos);
        if (!allowToXrayRender(renderOresESP[i].oreName) || currentBlock->isAir()) {
            renderOresESP.erase(renderOresESP.begin() + i);
            removedCount++;
			continue;
        }
    }
    
    if (removedCount >= 32) {
        rebuildChunkGeometry = true;
        removedCount = 0;
    }
} else {
    renderOresESP.clear();
}
	
	if (showPosition) {
	    playerPosX = playerPos->x;
	    playerPosY = playerPos->y;
	    playerPosY = playerPosY - 1.62;
	    playerPosZ = playerPos->z;
	}
	
	if (thiz->getHealth() == 0) {
		if (deathPosition) {
			vec3_t *deathPos = thiz->getPosition();
	        playerDeathPosX = deathPos->x;
	        playerDeathPosY = deathPos->y;
	        playerDeathPosY = playerDeathPosY - 1.62;
	        playerDeathPosZ = deathPos->z;
		}
		return old_NormalTick(thiz);
	}
	
	if (damageFly) {
		myGameMode->attack(thiz, true);
		myGameMode->attack(thiz, true);
		thiz->swing();
	}
	
	if (!isLANGame) {
		#if defined(__ARM_ARCH_7A__)
		LocalPlayer *EntityAccess = (LocalPlayer *)((char *)thiz + 4);
		#else
		LocalPlayer *EntityAccess = (LocalPlayer *)((char *)thiz + 8);
		#endif
	    if (strafe) {
			EntityAccess->setOnGround(1, true);
			EntityAccess->setIsJumping(0, true);
		}
		if (noJumpDelay) EntityAccess->setNoJumpDelay(1, 0);
		if (alwaysSprinting) thiz->setSprinting(true);
	    if (ladderFly) thiz->setClimbing(true);
	    if (step) {
	        thiz->initializeMaxAutoStep(1.0625);
	    } else {
		    thiz->initializeMaxAutoStep(0.5625);
	    }
		if (creativeFly) {
			thiz->getAbilities()->getLayer(2)->setAbility(10, true);
			activatedCreativeFly = true;
		} else if (activatedCreativeFly && !creativeFly) {
			if (thiz->isAttackableGamemode()) thiz->getAbilities()->getLayer(2)->setAbility(10, false);
			activatedCreativeFly = false;
		}
		if (forceCreativeFly) {
			thiz->getAbilities()->getLayer(2)->setAbility(9, true);
			activatedForceCreativeFly = true;
		} else if (activatedForceCreativeFly && !forceCreativeFly) {
			if (thiz->isAttackableGamemode()) thiz->getAbilities()->getLayer(2)->setAbility(9, false);
			activatedCreativeFly = false;
		}
	} else thiz->initializeMaxAutoStep(0.5625);
	
	std::vector<Mob*> actors = myLevel->getRuntimeActorList();
	nearestMob = NULL;
	fireballRotation = false;
	itemRotation = false;
	mobList.clear();
	
	for (Mob *object : actors) {
		if (object != thiz) {
			if (antiInvis) object->setInvisible(false);
			vec3_t *targetPos = object->getPosition();
			vec2_t *targetHitBox = object->getAABBDim();
			vec2_t calcTargetHitBox = *targetHitBox;
			float distanceToActor = mobDistanceTo(playerPos, targetPos, calcTargetHitBox.y);
			
			int ActorId = object->getEntityTypeId();
			if (distanceToActor <= 10 && ActorId == 4194389) {
			    if (antiFireball) {
					if (rotations) {
                        vec3_t calcObjPos = *targetPos;
                        auraAngle = globalPlayerPos.CalcPlayerAngle(calcObjPos);
						fireballRotation = true;
				    }
					myGameMode->attack(object, true);
			        if (swing) thiz->swing();
				}
				continue;
			} else if (distanceToActor <= 7 && ActorId == 64) {
			    if (killItems) {
					if (rotations) {
                        vec3_t calcObjPos = *targetPos;
                        auraAngle = globalPlayerPos.CalcPlayerAngle(calcObjPos);
						itemRotation = true;
				    }
				    myGameMode->attack(object, true);
			        if (swing) thiz->swing();
			    }
				continue;
			} else {
				if (mobAura) {
				    if (distanceToActor <= mobAuraRange && object->getHealth() != 0 && MobDetected(ActorId)) mobList.push_back(object);
                }
			}
		}
	}
	
	nearestTarget = NULL;
	nearestKillTarget = NULL;
	nearFriendList.clear();
	playerList.clear();
	nearPlayerList.clear();
	nearPlayer.clear();
	std::vector<Player*> nearPlayers;
	
    myLevel->getGameplayUserManager()->forEachActivePlayer([&](Player& objectt) {
        nearPlayers.push_back(&objectt);
		return true;
    }, true);

	for (Player *objectt : nearPlayers) {
		 if (objectt == thiz) continue;
		vec2_t* AABB = objectt->getAABBDim();
		const std::string PlayerName = objectt->getName();
		const std::string PlayerNameTag = objectt->getNameTag();
		vec2_t Hitbox = *AABB;
				
        if (antiBot) {
            if (PlayerName.length() <= 1) continue;
			if (PlayerNameTag.length() <= 1) continue;
            if (!AABB) continue;
            if (Hitbox.y < 0.05f || Hitbox.x < 0.05f || Hitbox.y > 2.1f || Hitbox.x > 10.9f) continue;
        }
		
			bool isFriend = false;
			bool isBotList = false;

			for (std::string currentPlayer : botList) {
				if (currentPlayer == PlayerName) {
				    isBotList = true;
					break;
				}
			}
			
			if (!isBotList) {
				for (std::string currentPlayer : friendList) {
				    if (currentPlayer == PlayerName) {
					    if (playerNear) nearFriendList.push_back(PlayerName);
				        isFriend = true;
						break;
				    }
			    }
			}
			
			bool isAttackable = objectt->isAttackableGamemode();
			if (!isFriend && !isBotList) {
				AABB->x = hitBox ? hitBoxValue : 0.6;
				nearPlayer.push_back(PlayerName);
			}
					
			int targetHealth = objectt->getHealth();
			vec3_t* targetPos = objectt->getPosition();
			
            nearPlayerList.push_back({PlayerName, PlayerNameTag, targetPos, Hitbox, targetHealth, objectt->getHurtTime(), objectt->getPlatform(), !isAttackable, isFriend, isBotList});
					
            if (killAura) {
				float distanceTo = hackDistanceTo(playerPos, targetPos);
				if (distanceTo <= rotationsRange) {
                    if (isAttackable && !isFriend && !isBotList && targetHealth != 0) {
                        if (rotations || targetLock) {
                            if (distanceTo <= rotationsRange) {
                                playerList.push_back(objectt);
                            }
                        } else {
                            if (distanceTo <= killRange) {
                                playerList.push_back(objectt);
                            }
                        }
                    }
			    }
            }
	}
	
		if (!mobList.empty()) {
			switch (auraType) {
				case 0: {
					if (rotations || targetLock) {
						vec3_t *objPos = mobList[0]->getPosition();
						vec2_t *hitBox = mobList[0]->getAABBDim();
                        vec3_t calcObjPos = *objPos;
						vec2_t calcHitBox = *hitBox;
                        auraAngle = globalPlayerPos.CalcMobAngle(calcObjPos, calcHitBox.y);
					}
					bool canHurt = true;
					if (hurtCheck) canHurt = mobList[0]->canBeHurted();
					if (canMobAuraDamage && canHurt) {
						myGameMode->attack(mobList[0], true);
                        if (swing) thiz->swing();
                        canMobAuraDamage = false;
                    }
				break;
				}
				case 1: {
					float nearestMobDistance = 9999;
                    for (Mob *potentialTarget : mobList) {
						if (potentialTarget != NULL) {
							vec2_t *potentialHitBox = potentialTarget->getAABBDim();
							vec2_t calcPotentialHitBox = *potentialHitBox;
							float potentialDistance = mobDistanceTo(playerPos, potentialTarget->getPosition(), calcPotentialHitBox.y);
                            if (potentialDistance < nearestMobDistance && potentialDistance <= mobAuraRange) {
                                nearestMobDistance = potentialDistance;
                                nearestMob = potentialTarget;
                            }
						}
                    }
					if (nearestMob != NULL) {
						//inMemoryFunction = nearestMob->getNameTag();
						//nearActorId = nearestMob->getEntityTypeId();
						if (rotations || targetLock) {
							vec3_t* objPos = nearestMob->getPosition();
							vec2_t *hitBox = nearestMob->getAABBDim();
                            vec3_t calcObjPos = *objPos;
							vec2_t calcHitBox = *hitBox;
                            auraAngle = globalPlayerPos.CalcMobAngle(calcObjPos, calcHitBox.y);
						}
						bool canHurt = true;
						if (hurtCheck) canHurt = nearestMob->canBeHurted();
                        if (canMobAuraDamage && canHurt) {
                            myGameMode->attack(nearestMob, true);
                            if (swing) thiz->swing();
                            canMobAuraDamage = false;
                        }
					}
                break;
                }
				case 2: {
					float nearestMobDistance = 9999;
                    for (Mob *potentialTarget : mobList) {
						if (potentialTarget != NULL) {
							vec2_t *potentialHitBox = potentialTarget->getAABBDim();
							vec2_t calcPotentialHitBox = *potentialHitBox;
							float potentialDistance = mobDistanceTo(playerPos, potentialTarget->getPosition(), calcPotentialHitBox.y);
                            if (potentialDistance < nearestMobDistance && potentialDistance <= mobAuraRange) {
                                nearestMobDistance = potentialDistance;
                                nearestMob = potentialTarget;
                            }
						}
                    }
					if (nearestMob != NULL) {
						if (rotations || targetLock) {
							vec3_t* objPos = nearestMob->getPosition();
							vec2_t *hitBox = nearestMob->getAABBDim();
                            vec3_t calcObjPos = *objPos;
							vec2_t calcHitBox = *hitBox;
                            auraAngle = globalPlayerPos.CalcMobAngle(calcObjPos, calcHitBox.y);
						}
					    for (Mob*actors : mobList) {
						    if (maxMobAuraTick > currentMobTick) {
						        currentMobTick = currentMobTick + 1;
					        } else {
								bool canHurt = true;
						        if (hurtCheck) canHurt = actors->canBeHurted();
								if (canHurt) {
									myGameMode->attack(actors, true);
					                if (swing) thiz->swing();
						            currentMobTick = 1;
								}
						    }
					    }
					}
				break;
				}
			}
		}

    if (!playerList.empty()) {
        switch (auraType) {
			case 0: {
				vec3_t* objPos = playerList[0]->getPosition();
				if (rotations || targetLock) {
                    vec3_t calcObjPos = *objPos;
                    auraAngle = globalPlayerPos.CalcPlayerAngle(calcObjPos);
				}
				if (hackDistanceTo(playerPos, objPos) <= killRange) {
					bool canHurt = true;
					if (hurtCheck) canHurt = playerList[0]->canBeHurted();
					if (canKillAuraDamage && canHurt) {
						if (!emulateNativeAttack) {
                            myGameMode->attack(playerList[0], true);
						} else {
							doHandleAttack = true;
						}
                        if (swing) thiz->swing();
                        canKillAuraDamage = false;
                    }
				}
			break;
			}
            case 1: {
                float nearestDistance = 9999;
                for (Player *potentialTarget : playerList) {
					if (potentialTarget != NULL) {
						float potentialDistance = hackDistanceTo(playerPos, potentialTarget->getPosition());
                        if (potentialDistance < nearestDistance && potentialDistance <= rotationsRange) {
                            nearestDistance = potentialDistance;
                            nearestTarget = potentialTarget;
							if (potentialDistance <= killRange) {
								nearestKillTarget = potentialTarget;
							}
                        }
					}
                }
				if (nearestTarget != NULL) {
					//inMemoryFunction = std::to_string(*(int *)((uint64_t)nearestTarget + 0x47C));
					if (rotations || targetLock) {
                        vec3_t* objPos = nearestTarget->getPosition();
                        vec3_t calcObjPos = *objPos;
                        auraAngle = globalPlayerPos.CalcPlayerAngle(calcObjPos);
					}
					if (nearestKillTarget != NULL) {
						bool canHurt = true;
					    if (hurtCheck) canHurt = nearestTarget->canBeHurted();
						if (canKillAuraDamage && canHurt) {
						    if (!emulateNativeAttack) {
                                myGameMode->attack(nearestTarget, true);
						    } else {
							    doHandleAttack = true;
						    }
                            if (swing) thiz->swing();
                            canKillAuraDamage = false;
                        }
					}
				}
            break;
			}
            case 2: {
				float nearestDistance = 9999;
                for (Player *potentialTarget : playerList) {
					if (potentialTarget != NULL) {
						float potentialDistance = hackDistanceTo(playerPos, potentialTarget->getPosition());
                        if (potentialDistance < nearestDistance && potentialDistance <= rotationsRange) {
                            nearestDistance = potentialDistance;
                            nearestTarget = potentialTarget;
							if (potentialDistance <= killRange) {
								nearestKillTarget = potentialTarget;
							}
                        }
					}
                }
				if (nearestTarget != NULL) {
					if (rotations || targetLock) {
                        vec3_t* objPos = nearestTarget->getPosition();
                        vec3_t calcObjPos = *objPos;
                        auraAngle = globalPlayerPos.CalcPlayerAngle(calcObjPos);
					}
				    if (nearestKillTarget != NULL) {
						for (Player *actors : playerList) {
							if (maxKillTick > currentKillTick) {
                                currentKillTick = currentKillTick + 1;
                            } else {
								bool canHurt = true;
						        if (hurtCheck) canHurt = actors->canBeHurted();
								if (canHurt) {
							        myGameMode->attack(actors, true);
                                    if (swing) thiz->swing();
							        currentKillTick = 1;
								}
						    }
						}
                    }
				}
            break;
			}
        }
    }
    return old_NormalTick(thiz);
}

void *(*old_PlayerAuthInputPacket)(void *a1, void *a2);
void *PlayerAuthInputPacket(void *a1, void *a2) {
	if (panic) return old_PlayerAuthInputPacket(a1, a2);
	if (!inMenu) {
		if (!isLANGame) {
		if (derp) {
		    if (derpGradys < 360) {
			    derpGradys = derpGradys + 30;
		    } else {
			    derpGradys = 0;
		    }
			#if defined(__ARM_ARCH_7A__)
			*(float *)((int)a1 + 36) = derpGradys;
			#else
			*(float *)((__int64)a1 + 48) = derpGradys;
			#endif
		}
		if (rotations) {
			if (fireballRotation || itemRotation || !mobList.empty() || !playerList.empty()) {
				#if defined(__ARM_ARCH_7A__)
				*(float *)((int)a1 + 36) = auraAngle.y;
				#else
				*(float *)((__int64)a1 + 48) = auraAngle.y;
				#endif
			}
		}
		}
	}
	return old_PlayerAuthInputPacket(a1, a2);
}

void *(*old_ClientInstance_onTick)(ClientInstance *thiz, void *a2, void *a3);
void *ClientInstance_onTick(ClientInstance *thiz, void *a2, void *a3) {
	CI = thiz;
	std::string currentScreenName = thiz->getScreenName();
	//inMemoryFunction = currentScreenName;
	if (currentScreenName == oxorany("hud_screen")) {
		inHudScreen = true;
		inMenu = false;
	} else {
		inHudScreen = false;
	}
	if (currentScreenName == oxorany("chat_screen")) {
		inChatScreen = true;
	} else {
		inChatScreen = false;
	}
	if (currentScreenName == oxorany("/hbui/index.html")) {
		manageWorldsScreen = true;
	} else {
		manageWorldsScreen = false;
	}
	if (panic) return old_ClientInstance_onTick(thiz, a2, a3);
	
	if (onFunctionSound) {
		thiz->playUI(oxorany("random.pop"), 0.2, 2);
		onFunctionSound = false;
	}
	if (offFunctionSound) {
		thiz->playUI(oxorany("random.pop2"), 0.2, 2);
		offFunctionSound = false;
	}
	if (errorFunctionSound) {
		thiz->playUI(oxorany("random.break"), 0.2, 2);
		errorFunctionSound = false;
	}
	if (inMenu) return old_ClientInstance_onTick(thiz, a2, a3);
	
	LevelRenderer *thisLevel = thiz->getLevelRenderer();
	//inMemoryFunction = format((void *)thiz);
	//inMemoryFunction = format((void *)thisLevel->getLevelRendererPlayer());
	
	if (rebuildChunkGeometry) {
		thisLevel->rebuildAllRenderChunkGeometry();
		rebuildChunkGeometry = false;
	}

	if (doHandleAttack) {
		thiz->handleDestoryOrAttackButtonPress();
		doHandleAttack = false;
	}
	
	if (autoClicker) {
		if (canClick) {
			thiz->handleDestoryOrAttackButtonPress();
			canClick = false;
		}
	}
    return old_ClientInstance_onTick(thiz, a2, a3);
}

void *(*old_HudPlayerRenderer_update)(void *a1, void *a2, void *a3, void *a4, void *a5, void *a6, void *a7, void *a8, void *a9, void *a10);
void *HudPlayerRenderer_update(void *a1, void *a2, void *a3, void *a4, void *a5, void *a6, void *a7, void *a8, void *a9, void *a10) {
	if (!panic) {
		if (showPaperdoll) {
			#if defined(__ARM_ARCH_7A__)
			*(float *)((int)a1 + 8) = 0.3;
			#else
			*(float *)((__int64)a1 + 12) = 0.3;
			#endif
		}
	}
    return old_HudPlayerRenderer_update(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10);
}

void *(*old_InLanGame1)(void *a1, void *a2, void *a3, void *a4, void *a5, void *a6, void *a7, void *a8, void *a9, void *a10);
void *InLanGame1(void *a1, void *a2, void *a3, void *a4, void *a5, void *a6, void *a7, void *a8, void *a9, void *a10) {
	if (!inMenu) isLANGame = true;
    return old_InLanGame1(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10);
}

void *(*old_InLanGame2)(void *a1, void *a2, void *a3, void *a4, void *a5, void *a6, void *a7, void *a8, void *a9, void *a10);
void *InLanGame2(void *a1, void *a2, void *a3, void *a4, void *a5, void *a6, void *a7, void *a8, void *a9, void *a10) {
	if (!inMenu) isLANGame = true;
    return old_InLanGame2(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10);
}

void **(*old_InMainMenu)(void **a1, void **a2, void **a3, void **a4, void **a5, void **a6, void **a7, void **a8, void **a9, void **a10);
void **InMainMenu(void **a1, void **a2, void **a3, void **a4, void **a5, void **a6, void **a7, void **a8, void **a9, void **a10) {
	inMenu = true;
	isLANGame = false;
	nearestTarget = NULL;
	nearestKillTarget = NULL;
	nearestMob = NULL;
	localPlayer = NULL;
	fireballRotation = false;
	itemRotation = false;
	playerList.clear();
	nearPlayerList.clear();
	nearPlayer.clear();
	nearFriendList.clear();
	mobList.clear();
	renderOresESP.clear();
    return old_InMainMenu(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10);
}

int (*old_LevelRendererCamera_FogState)(void *a1);
int LevelRendererCamera_FogState(void *a1) {
	if (!panic) {
		if (fogState != 0) return fogState - 1;
	}
    return old_LevelRendererCamera_FogState(a1);
}

float (*old_PlayerFov)(void *a1, void *a2, void *a3, void *a4);
float PlayerFov(void *a1, void *a2, void *a3, void *a4) {
	if (!panic) {
		if (enableFov) return globalPlayerFov =  old_PlayerFov(a1, a2, a3, a4) + fov;
	}
	return globalPlayerFov = old_PlayerFov(a1, a2, a3, a4);
}

void (*old_DropItemTime)(void *a1);
void DropItemTime(void *a1) {
	if (panic) return old_DropItemTime(a1);
	if (fastDrop) {
	    #if defined(__ARM_ARCH_7A__)
		float currentTime = *(float *)((int)a1 + 436);
	    if (currentTime < 35) *(float *)((int)a1 + 436) = 35;
		#else
		float currentTime = *(float *)((__int64)a1 + 692);
	    if (currentTime < 35) *(float *)((__int64)a1 + 692) = 35;
		#endif
	}
    return old_DropItemTime(a1);
}

void (*old_Gravity)(...);
#if defined(__ARM_ARCH_7A__)
void Gravity(void *a1, void *a2, void *a3, int a4, void *a5, void *a6) {
#else
void Gravity(void *a1, void *a2, void *a3, __int64 a4, void *a5, void *a6) {
#endif
    if (panic) return old_Gravity(a1, a2, a3, a4, a5, a6);
    if (!isLANGame) {
		float mine = *(float *)(a4 + 28);
	    *(float *)(a4 + 28) = mine + gravityValue + 0.08;
		if (fastFall) {
			if (mine < 0) *(float *)(a4 + 28) = -99;
		}
	}
    return old_Gravity(a1, a2, a3, a4, a5, a6);
}

int (*old_AppPlatform_getMaxPlayers)(void *thiz);
int AppPlatform_getMaxPlayers(void *thiz) {
	if (!panic) return myMaxPlayers;
    return old_AppPlatform_getMaxPlayers(thiz);
}

bool (*old_Entitlement_UnlockMarket)(void *a1, void *a2);
bool Entitlement_UnlockMarket(void *a1, void *a2) {
	if (!panic) {
		if (unlockMarket && !manageWorldsScreen) return true;
	}
    return old_Entitlement_UnlockMarket(a1, a2);
}

int (*old_GameMode_FastBridge)(int a1, int a2, int *a3, int a4, int a5, int a6);
int GameMode_FastBridge(int a1, int a2, int *a3, int a4, int a5, int a6) {
	if (panic) return old_GameMode_FastBridge(a1, a2, a3, a4, a5, a6);
	if (fastBridge) {
		old_GameMode_FastBridge(a1, a2, a3, a4, a5, a6);
		return 0;
	}
    return old_GameMode_FastBridge(a1, a2, a3, a4, a5, a6);
}

float (*old_GameMode_BlockReach)(void *a1, void *a2, void *a3);
float GameMode_BlockReach(void *a1, void *a2, void *a3) {
	if (!panic) {
		if (blockReach) return 12;
	}
    return old_GameMode_BlockReach(a1, a2, a3);
}

void *(*old_LevelRendererPlayer_setupFog)(void *a1, void *a2, void *a3);
void *LevelRendererPlayer_setupFog(void *a1, void *a2, void *a3) {
	if (panic) return old_LevelRendererPlayer_setupFog(a1, a2, a3);
	if (antiBlind) {
		#if defined(__ARM_ARCH_7A__)
		*(bool *)((int)a1 + 1682) = true;
		#else
		*(bool *)((__int64)a1 + 2450) = true;
		#endif
	}
    return old_LevelRendererPlayer_setupFog(a1, a2, a3);
}

void (*old_SpeedHack)(void *a1, void *a2, void *a3, void *a4, void *a5, void *a6, void *a7, void *a8, void *a9, void *a10, void *a11, void *a12, void *a13);
void SpeedHack(void *a1, void *a2, void *a3, void *a4, void *a5, void *a6, void *a7, void *a8, void *a9, void *a10, void *a11, void *a12, void *a13) {
	if (panic) return old_SpeedHack(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13);
	if (!isLANGame) {
	    for (int i = 0; i < speedValue - 1; ++i) {
		    old_SpeedHack(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13);
        }
	}
    return old_SpeedHack(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13);
}

float (*old_JumpHack)(void *a1, void *a2, void *a3, void *a4, void *a5, void *a6);
float JumpHack(void *a1, void *a2, void *a3, void *a4, void *a5, void *a6) {
	if (panic) return 0.42;
	if (!isLANGame) {
	    if (jumpValue != 1) {
		    return 0.42 + ((jumpValue - 1) / 10);
	    }
	}
    return 0.42;
}

void (*old_WaterSpeed)(void *a1, void *a2, void *a3, void *a4, void *a5, void *a6);
void WaterSpeed(void *a1, void *a2, void *a3, void *a4, void *a5, void *a6) {
	if (panic) return old_WaterSpeed(a1, a2, a3, a4, a5, a6);
	if (!isLANGame) {
	    for (int i = 0; i < waterSpeedValue - 1; ++i) {
		    old_WaterSpeed(a1, a2, a3, a4, a5, a6);
        }
	}
    return old_WaterSpeed(a1, a2, a3, a4, a5, a6);
}

void (*old_LavaSpeed)(void *a1, void *a2, void *a3, void *a4, void *a5, void *a6);
void LavaSpeed(void *a1, void *a2, void *a3, void *a4, void *a5, void *a6) {
	if (panic) return old_LavaSpeed(a1, a2, a3, a4, a5, a6);
	if (!isLANGame) {
	    for (int i = 0; i < lavaSpeedValue - 1; ++i) {
		    old_LavaSpeed(a1, a2, a3, a4, a5, a6);
        }
	}
    return old_LavaSpeed(a1, a2, a3, a4, a5, a6);
}


#if defined(__ARM_ARCH_7A__)
int *(*old_WasInWater)(_DWORD *a1, int a2);
int *WasInWater(_DWORD *a1, int a2) {
	if (panic) return old_WasInWater(a1, a2);
	if (!isLANGame) {
	    if (!inMenu && offWaterTrigger) {
		    *(_DWORD *)(a1[7] + 4 * ((((unsigned int)(a1[8] - a1[7]) >> 2) - 1) & a2)) = -1;
	    }
	}
	return old_WasInWater(a1, a2);
}

int *(*old_ActorHeadInWater)(_DWORD *a1, int a2);
int *ActorHeadInWater(_DWORD *a1, int a2) {
	if (panic) return old_ActorHeadInWater(a1, a2);
	if (!isLANGame) {
	    if (!inMenu && offWaterTrigger) {
		    *(_DWORD *)(a1[7] + 4 * ((((unsigned int)(a1[8] - a1[7]) >> 2) - 1) & a2)) = -1;
	    }
	}
	return old_ActorHeadInWater(a1, a2);
}

int *(*old_WaterSplashEffect)(_DWORD *a1, int a2);
int *WaterSplashEffect(_DWORD *a1, int a2) {
	if (panic) return old_WaterSplashEffect(a1, a2);
	if (!isLANGame) {
	    if (!inMenu && offWaterTrigger) {
		    *(_DWORD *)(a1[7] + 4 * ((((unsigned int)(a1[8] - a1[7]) >> 2) - 1) & a2)) = -1;
	    }
	}
	return old_WaterSplashEffect(a1, a2);
}


#else


__int64 (*old_WasInWater)(_QWORD *a1, int a2);
__int64 WasInWater(_QWORD *a1, int a2) {
	if (panic) return old_WasInWater(a1, a2);
	if (!isLANGame) {
	    if (!inMenu && offWaterTrigger) {
		    *(_QWORD *)(a1[7] + 8LL * (((unsigned int)((unsigned __int64)(a1[8] - a1[7]) >> 3) - 1) & a2)) = -1;
	    }
	}
	return old_WasInWater(a1, a2);
}

__int64 (*old_ActorHeadInWater)(_QWORD *a1, int a2);
__int64 ActorHeadInWater(_QWORD *a1, int a2) {
	if (panic) return old_ActorHeadInWater(a1, a2);
	if (!isLANGame) {
	    if (!inMenu && offWaterTrigger) {
		    *(_QWORD *)(a1[7] + 8LL * (((unsigned int)((unsigned __int64)(a1[8] - a1[7]) >> 3) - 1) & a2)) = -1;
	    }
	}
	return old_ActorHeadInWater(a1, a2);
}

__int64 (*old_WaterSplashEffect)(_QWORD *a1, int a2);
__int64 WaterSplashEffect(_QWORD *a1, int a2) {
	if (panic) return old_WaterSplashEffect(a1, a2);
	if (!isLANGame) {
	    if (!inMenu && offWaterTrigger) {
		    *(_QWORD *)(a1[7] + 8LL * (((unsigned int)((unsigned __int64)(a1[8] - a1[7]) >> 3) - 1) & a2)) = -1;
	    }
	}
	return old_WaterSplashEffect(a1, a2);
}
#endif

bool canDamageGeyser = true;
void (*old_TitleMessage_setSubtitle)(void *, std::string, void *);
void TitleMessage_setSubtitle(void *a1, std::string a2, void *a3) {
	if (canDamageGeyser) {
		if (useGeyserCooldown) {
		     if (a2 == oxorany("§8˙˙˙˙˙˙˙˙§7˙˙") || a2 == oxorany("§8˙˙˙˙˙˙˙˙˙§7˙") || a2 == oxorany("§8˙˙˙˙˙˙˙˙˙˙§7")) {
		        canKillAuraDamage = true;
		        canMobAuraDamage = true;
		        canDamageGeyser = false;
	        }
		}
	} else {
		canDamageGeyser = true;
	}
	return old_TitleMessage_setSubtitle(a1, a2, a3);
}

unsigned __int8 (*old_Block_getRenderLayer)(Block *thiz, void *a2, const vec3_ti &blockPos);
unsigned __int8 Block_getRenderLayer(Block *thiz, void *a2, const vec3_ti &blockPos) {
    if (inMenu) return old_Block_getRenderLayer(thiz, a2, blockPos);
	if (xrayDefault && !panic) {
    BlockType *v5 = thiz->getBlockTypeClassCustom();
    if (!v5) return old_Block_getRenderLayer(thiz, a2, blockPos);

    #if defined(__ARM_ARCH_7A__)
    const std::string *blockName = (const std::string *)((char *)v5 + 4);
    #else
    const std::string *blockName = (const std::string *)((char *)v5 + 8);
    #endif

    if (!blockName || blockName->empty()) return old_Block_getRenderLayer(thiz, a2, blockPos);

    std::string blockView = *blockName;
    vec3_t orePosition = {(float)blockPos.x, (float)blockPos.y, (float)blockPos.z};
    vec3_t camPos = {CameraPos.x, CameraPos.y, CameraPos.z};
    float distanceToBlock = distanceTo(&camPos, &orePosition);
    
    if (allowToXrayRender(blockView)) {
        if (distanceToBlock <= 32.0f) {
            bool alreadyExists = false;
            for (XrayOres existingOre : renderOresESP) {
                if (existingOre.orePos.x == blockPos.x && 
                    existingOre.orePos.y == blockPos.y && 
                    existingOre.orePos.z == blockPos.z) {
                    alreadyExists = true;
                    break;
                }
            }
            
            if (!alreadyExists) {
                renderOresESP.push_back({blockView, blockPos, {(float)blockPos.x, (float)blockPos.y, (float)blockPos.z}, GetOreColor(blockView)});
            }
        } 
    }
	}
    return old_Block_getRenderLayer(thiz, a2, blockPos);
}

void (*old_BaseActorRenderer_renderText)(void *a1, void *a2, void *a3, void *a4, void *a5, void *a6);
void BaseActorRenderer_renderText(void *a1, void *a2, void *a3, void *a4, void *a5, void *a6) {
	if (panic) return old_BaseActorRenderer_renderText(a1, a2, a3, a4, a5, a6);
	if (esp && espNametag && inHudScreen) {
		std::string currentNameTag = *static_cast<std::string*>(a3);
	    for (PlayerStruct player : nearPlayerList) {
			if (player.isBot) continue;
			if (currentNameTag.find(player.nickname) != std::string::npos || currentNameTag == player.nameTag) {
				return;
			}
	    }
	}
    return old_BaseActorRenderer_renderText(a1, a2, a3, a4, a5, a6);
}



#include "hotkeys.h"




// ---------------------------------------------------------------------------
// A64HookSafe -- never hand A64HookFunction a null target.
//
// getAbsoluteAddress(MineLib, 0) returns the LIBRARY BASE, not "invalid".
// Several Offsets64 entries are 0 (unrecovered). Hook-installing one of those
// writes a trampoline over the ELF header of libminecraftpe.so -> SIGSEGV.
// Skipping leaves the feature inert instead of crashing the client.
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// Hook-site verification for the 1.26.66.1 body-layer table.
//
// Each recovered offset must land on real, executable AArch64 code. We check:
//   * the page is mapped r-x
//   * the first instruction decodes to a plausible function entry
//     (stp / sub sp / str / mov) -- a wrong offset almost always lands on data
//     or mid-instruction garbage
// Reports PASS/FAIL per hook to logcat under tag "ApollonOffset" so the device
// can tell us which entries actually resolved.
// ---------------------------------------------------------------------------
// Executable-range lookup straight from /proc/self/maps, so we never read a
// non-executable page (segfault) while validating an offset.
static bool ApollonIsExec(uintptr_t addr) {
    FILE *fp = fopen("/proc/self/maps", "rt");
    if (!fp) return false;
    char line[512];
    bool ok = false;
    while (fgets(line, sizeof(line), fp)) {
        unsigned long lo = 0, hi = 0; char perms[8] = {0};
        if (sscanf(line, "%lx-%lx %7s", &lo, &hi, perms) != 3) continue;
        if (addr >= lo && addr < hi) { ok = (perms[2] == 'x'); break; }
    }
    fclose(fp);
    return ok;
}

// A real function entry. AArch64 entries start with a frame setup, a stack
// adjustment, a store, or a register move. Garbage / data pages do not.
static bool ApollonVerifyOne(uintptr_t relOffset, const char *what) {
    uintptr_t addr = getAbsoluteAddress(MineLib, relOffset);
    if (!addr) return false;
    if (!ApollonIsExec(addr)) {
        __android_log_print(ANDROID_LOG_ERROR, "ApollonOffset",
                            "NOT-EXEC %s @ 0x%lx", what ? what : "?", (unsigned long)relOffset);
        return false;
    }
    uint32_t w = *(volatile uint32_t *)addr;
    // STP/STR/LDR/SUB/ADD/MOV/MOVZ/MOVI families that legitimately open a function
    bool stp   = (w & 0xFFC00000u) == 0xA9800000u;   // stp ..., [sp, #-N]!  (pre-index)
    bool stp2  = (w & 0xFF800000u) == 0xA9000000u;   // stp (offset form)
    bool sub   = (w & 0xFFC003FFu) == 0xD10003FFu;   // sub sp, sp, #imm
    bool str   = (w & 0xFFC00000u) == 0xB9000000u;   // str (32-bit)
    bool strx  = (w & 0xFFC00000u) == 0xF9000000u;   // str (64-bit)
    bool mov   = (w & 0xFFE0FFE0u) == 0x2A0003E0u;   // mov wD, wS
    bool movx  = (w & 0xFFE0FFE0u) == 0xAA0003E0u;   // mov xD, xS
    bool movz  = (w & 0xFF800000u) == 0x52800000u;   // movz wD
    bool movi  = (w & 0xFFF8FC00u) == 0x0F000400u;   // movi d/v
    bool ldr   = (w & 0xFFC00000u) == 0xF9400000u;   // ldr x
    bool ldrb  = (w & 0xFFC00000u) == 0x39400000u;   // ldrb
    bool ok = stp || stp2 || sub || str || strx || mov || movx || movz || movi || ldr || ldrb;
    __android_log_print(ok ? ANDROID_LOG_INFO : ANDROID_LOG_ERROR, "ApollonOffset",
                        "%s %s @ 0x%lx  first=%08x",
                        ok ? "PASS" : "FAIL", what ? what : "?",
                        (unsigned long)relOffset, w);
    return ok;
}

static inline void A64HookSafe(uintptr_t relOffset, void *replace, void **orig,
                               const char *what = nullptr) {
    if (relOffset == 0) {
        if (orig) *orig = nullptr;
        return;
    }
    if (!ApollonVerifyOne(relOffset, what)) {
        __android_log_print(ANDROID_LOG_ERROR, "ApollonOffset",
                            "REFUSED %s @ %p : not a plausible function entry",
                            what ? what : "?", (void *)relOffset);
        if (orig) *orig = nullptr;
        return;
    }
    A64HookFunction((void *)getAbsoluteAddress(MineLib, relOffset), replace, orig);
    __android_log_print(ANDROID_LOG_INFO, "ApollonOffset",
                        "HOOKED %s @ 0x%lx", what ? what : "?", (unsigned long)relOffset);
}

pthread_t cheatThread;
bool isHackLoaded = false;
void *hack_thread(void *) {
    if (!isHackLoaded) {
    ProcMap mineMap;
    do {
        mineMap = KittyMemory::getLibraryMap(MineLib);
		handle = dlopen(OBFUSCATE("libminecraftpe.so"), RTLD_LAZY);
    } while (!mineMap.isValid());
	
	RenderHook();
	
	#if defined(__aarch64__)

    DobbyHook(reinterpret_cast<void*>((const void*(*)(...))dlsym(handle, OBFUSCATE("Java_com_mojang_minecraftpe_MainActivity_nativeKeyHandler"))), reinterpret_cast<void*>(new_Java_com_mojang_minecraftpe_MainActivity_nativeKeyHandler), reinterpret_cast<void**>(&old_Java_com_mojang_minecraftpe_MainActivity_nativeKeyHandler));
	hexPatches.NativeKeyHandler = MemoryPatch::createWithHex(MineLib, Offsets64::NativeKeyHandler, "C0 03 5F D6");
	__android_log_print(hexPatches.NativeKeyHandler.isValid()?ANDROID_LOG_INFO:ANDROID_LOG_WARN, "ApollonOffset", "PATCH-%s %s @ 0x%lx", hexPatches.NativeKeyHandler.isValid()?"ok":"skip", "NativeKeyHandler", (unsigned long)Offsets64::NativeKeyHandler);
	hexPatches.NativeKeyHandler.Modify();
	
	A64HookSafe(Offsets64::NoNametags,  (void *) BaseActorRenderer_renderText, (void **) &old_BaseActorRenderer_renderText, "NoNametags");
	
	hexPatches.NoCaveVignette = MemoryPatch::createWithHex(MineLib, Offsets64::VignetteRenderer, "C0 03 5F D6");
	__android_log_print(hexPatches.NoCaveVignette.isValid()?ANDROID_LOG_INFO:ANDROID_LOG_WARN, "ApollonOffset", "PATCH-%s %s @ 0x%lx", hexPatches.NoCaveVignette.isValid()?"ok":"skip", "VignetteRenderer", (unsigned long)Offsets64::VignetteRenderer);
	
	hexPatches.AntiKnockBack = MemoryPatch::createWithHex(MineLib, Offsets64::LerpMotion, "C0 03 5F D6");
	__android_log_print(hexPatches.AntiKnockBack.isValid()?ANDROID_LOG_INFO:ANDROID_LOG_WARN, "ApollonOffset", "PATCH-%s %s @ 0x%lx", hexPatches.AntiKnockBack.isValid()?"ok":"skip", "LerpMotion", (unsigned long)Offsets64::LerpMotion);
	
	hexPatches.FullBright = MemoryPatch::createWithHex(MineLib, Offsets64::FullBright, "88 56 A8 52 00 01 27 1E C0 03 5F D6");
	__android_log_print(hexPatches.FullBright.isValid()?ANDROID_LOG_INFO:ANDROID_LOG_WARN, "ApollonOffset", "PATCH-%s %s @ 0x%lx", hexPatches.FullBright.isValid()?"ok":"skip", "FullBright", (unsigned long)Offsets64::FullBright);//return 999.0f
	A64HookSafe(Offsets64::Block_getRenderLayer,  (void *) Block_getRenderLayer, (void **) &old_Block_getRenderLayer, "Block_getRenderLayer");
	
    A64HookSafe(Offsets64::ClientInstanceOnTick,  (void *) ClientInstance_onTick, (void **) &old_ClientInstance_onTick, "ClientInstanceOnTick");
	
    A64HookSafe(Offsets64::Subtitle,  (void *) TitleMessage_setSubtitle, (void **) &old_TitleMessage_setSubtitle, "Subtitle");
	
    A64HookSafe(Offsets64::ShowPaperdoll,  (void *) HudPlayerRenderer_update, (void **) &old_HudPlayerRenderer_update, "ShowPaperdoll");
	
    A64HookSafe(Offsets64::SetupFog,  (void *) LevelRendererPlayer_setupFog, (void **) &old_LevelRendererPlayer_setupFog, "SetupFog");
	
    A64HookSafe(Offsets64::GetMaxPlayers,  (void *) AppPlatform_getMaxPlayers, (void **) &old_AppPlatform_getMaxPlayers, "GetMaxPlayers");
	
    A64HookSafe(Offsets64::UnlockMarket,  (void *) Entitlement_UnlockMarket, (void **) &old_Entitlement_UnlockMarket, "UnlockMarket");
	
	A64HookSafe(Offsets64::InMainMenu,  (void *) InMainMenu, (void **) &old_InMainMenu, "InMainMenu");
	A64HookSafe(Offsets64::InLanGame1,  (void *) InLanGame1, (void **) &old_InLanGame1, "InLanGame1");
	A64HookSafe(Offsets64::InLanGame2,  (void *) InLanGame2, (void **) &old_InLanGame2, "InLanGame2");
	
	A64HookSafe(Offsets64::Fov,  (void *) PlayerFov, (void **) &old_PlayerFov, "Fov");

	A64HookSafe(Offsets64::FogState,  (void *) LevelRendererCamera_FogState, (void **) &old_LevelRendererCamera_FogState, "FogState");

	hexPatches.Font0 = MemoryPatch::createWithHex(MineLib, Offsets64::Font, "00 00 00 3F");
	__android_log_print(hexPatches.Font0.isValid()?ANDROID_LOG_INFO:ANDROID_LOG_WARN, "ApollonOffset", "PATCH-%s %s @ 0x%lx", hexPatches.Font0.isValid()?"ok":"skip", "Font", (unsigned long)Offsets64::Font);//0
	hexPatches.Font1 = MemoryPatch::createWithHex(MineLib, Offsets64::Font, "9A 99 19 3F");
	__android_log_print(hexPatches.Font1.isValid()?ANDROID_LOG_INFO:ANDROID_LOG_WARN, "ApollonOffset", "PATCH-%s %s @ 0x%lx", hexPatches.Font1.isValid()?"ok":"skip", "Font", (unsigned long)Offsets64::Font);//1
	hexPatches.Font2 = MemoryPatch::createWithHex(MineLib, Offsets64::Font, "33 33 33 3F");
	__android_log_print(hexPatches.Font2.isValid()?ANDROID_LOG_INFO:ANDROID_LOG_WARN, "ApollonOffset", "PATCH-%s %s @ 0x%lx", hexPatches.Font2.isValid()?"ok":"skip", "Font", (unsigned long)Offsets64::Font);//2
	hexPatches.Font3 = MemoryPatch::createWithHex(MineLib, Offsets64::Font, "CD CC 4C 3F");
	__android_log_print(hexPatches.Font3.isValid()?ANDROID_LOG_INFO:ANDROID_LOG_WARN, "ApollonOffset", "PATCH-%s %s @ 0x%lx", hexPatches.Font3.isValid()?"ok":"skip", "Font", (unsigned long)Offsets64::Font);//3
	hexPatches.Font4 = MemoryPatch::createWithHex(MineLib, Offsets64::Font, "66 66 66 3F");
	__android_log_print(hexPatches.Font4.isValid()?ANDROID_LOG_INFO:ANDROID_LOG_WARN, "ApollonOffset", "PATCH-%s %s @ 0x%lx", hexPatches.Font4.isValid()?"ok":"skip", "Font", (unsigned long)Offsets64::Font);//4
	hexPatches.Font5 = MemoryPatch::createWithHex(MineLib, Offsets64::Font, "00 00 80 3F");
	__android_log_print(hexPatches.Font5.isValid()?ANDROID_LOG_INFO:ANDROID_LOG_WARN, "ApollonOffset", "PATCH-%s %s @ 0x%lx", hexPatches.Font5.isValid()?"ok":"skip", "Font", (unsigned long)Offsets64::Font);//5
	hexPatches.Font6 = MemoryPatch::createWithHex(MineLib, Offsets64::Font, "CD CC 8C 3F");
	__android_log_print(hexPatches.Font6.isValid()?ANDROID_LOG_INFO:ANDROID_LOG_WARN, "ApollonOffset", "PATCH-%s %s @ 0x%lx", hexPatches.Font6.isValid()?"ok":"skip", "Font", (unsigned long)Offsets64::Font);//6
	hexPatches.Font7 = MemoryPatch::createWithHex(MineLib, Offsets64::Font, "9A 99 99 3F");
	__android_log_print(hexPatches.Font7.isValid()?ANDROID_LOG_INFO:ANDROID_LOG_WARN, "ApollonOffset", "PATCH-%s %s @ 0x%lx", hexPatches.Font7.isValid()?"ok":"skip", "Font", (unsigned long)Offsets64::Font);//7
	hexPatches.Font8 = MemoryPatch::createWithHex(MineLib, Offsets64::Font, "66 66 A6 3F");
	__android_log_print(hexPatches.Font8.isValid()?ANDROID_LOG_INFO:ANDROID_LOG_WARN, "ApollonOffset", "PATCH-%s %s @ 0x%lx", hexPatches.Font8.isValid()?"ok":"skip", "Font", (unsigned long)Offsets64::Font);//8
	hexPatches.Font9 = MemoryPatch::createWithHex(MineLib, Offsets64::Font, "33 33 B3 3F");
	__android_log_print(hexPatches.Font9.isValid()?ANDROID_LOG_INFO:ANDROID_LOG_WARN, "ApollonOffset", "PATCH-%s %s @ 0x%lx", hexPatches.Font9.isValid()?"ok":"skip", "Font", (unsigned long)Offsets64::Font);//9
	hexPatches.Font10 = MemoryPatch::createWithHex(MineLib, Offsets64::Font, "00 00 C0 3F");
	__android_log_print(hexPatches.Font10.isValid()?ANDROID_LOG_INFO:ANDROID_LOG_WARN, "ApollonOffset", "PATCH-%s %s @ 0x%lx", hexPatches.Font10.isValid()?"ok":"skip", "Font", (unsigned long)Offsets64::Font);//10
	
	A64HookSafe(Offsets64::FastDrop,  (void *) DropItemTime, (void **) &old_DropItemTime, "FastDrop");
	
	hexPatches.NoHurtCam = MemoryPatch::createWithHex(MineLib, Offsets64::NoHurtCam, "C0 03 5F D6");
	__android_log_print(hexPatches.NoHurtCam.isValid()?ANDROID_LOG_INFO:ANDROID_LOG_WARN, "ApollonOffset", "PATCH-%s %s @ 0x%lx", hexPatches.NoHurtCam.isValid()?"ok":"skip", "NoHurtCam", (unsigned long)Offsets64::NoHurtCam);
	
	hexPatches.NoCamDistortion = MemoryPatch::createWithHex(MineLib, Offsets64::NoCamDistortion, "C0 03 5F D6");
	__android_log_print(hexPatches.NoCamDistortion.isValid()?ANDROID_LOG_INFO:ANDROID_LOG_WARN, "ApollonOffset", "PATCH-%s %s @ 0x%lx", hexPatches.NoCamDistortion.isValid()?"ok":"skip", "NoCamDistortion", (unsigned long)Offsets64::NoCamDistortion);
	
	hexPatches.NoBoatRotation = MemoryPatch::createWithHex(MineLib, Offsets64::NoBoatRotation, "C0 03 5F D6");
	__android_log_print(hexPatches.NoBoatRotation.isValid()?ANDROID_LOG_INFO:ANDROID_LOG_WARN, "ApollonOffset", "PATCH-%s %s @ 0x%lx", hexPatches.NoBoatRotation.isValid()?"ok":"skip", "NoBoatRotation", (unsigned long)Offsets64::NoBoatRotation);
	
	hexPatches.NoCamSleep = MemoryPatch::createWithHex(MineLib, Offsets64::NoCamSleep, "C0 03 5F D6");
	__android_log_print(hexPatches.NoCamSleep.isValid()?ANDROID_LOG_INFO:ANDROID_LOG_WARN, "ApollonOffset", "PATCH-%s %s @ 0x%lx", hexPatches.NoCamSleep.isValid()?"ok":"skip", "NoCamSleep", (unsigned long)Offsets64::NoCamSleep);
	
	hexPatches.PlaceCamera = MemoryPatch::createWithHex(MineLib, Offsets64::PlaceCamera, "C0 03 5F D6");
	__android_log_print(hexPatches.PlaceCamera.isValid()?ANDROID_LOG_INFO:ANDROID_LOG_WARN, "ApollonOffset", "PATCH-%s %s @ 0x%lx", hexPatches.PlaceCamera.isValid()?"ok":"skip", "PlaceCamera", (unsigned long)Offsets64::PlaceCamera);
	
	A64HookSafe(Offsets64::SpeedHack,  (void *) SpeedHack, (void **) &old_SpeedHack, "SpeedHack");
	
	A64HookSafe(Offsets64::NormalTick,  (void*) NormalTick, (void**) &old_NormalTick, "NormalTick");
	A64HookSafe(Offsets64::Intersects,  (void*) Intersects, (void**) &old_Intersects, "Intersects");
	A64HookSafe(Offsets64::PlayerAuthInput,  (void *) PlayerAuthInputPacket, (void **) &old_PlayerAuthInputPacket, "PlayerAuthInput");
	
	A64HookSafe(Offsets64::WaterTrigger1,  (void *) WasInWater, (void **) &old_WasInWater, "WaterTrigger1");
	A64HookSafe(Offsets64::WaterTrigger2,  (void *) ActorHeadInWater, (void **) &old_ActorHeadInWater, "WaterTrigger2");
	A64HookSafe(Offsets64::WaterTrigger3,  (void *) WaterSplashEffect, (void **) &old_WaterSplashEffect, "WaterTrigger3");
	
	hexPatches.SlowDownTriggers = MemoryPatch::createWithHex(MineLib, Offsets64::SlowDownTriggers, "C0 03 5F D6");
	__android_log_print(hexPatches.SlowDownTriggers.isValid()?ANDROID_LOG_INFO:ANDROID_LOG_WARN, "ApollonOffset", "PATCH-%s %s @ 0x%lx", hexPatches.SlowDownTriggers.isValid()?"ok":"skip", "SlowDownTriggers", (unsigned long)Offsets64::SlowDownTriggers);
	
	A64HookSafe(Offsets64::JumpHack,  (void *) JumpHack, (void **) &old_JumpHack, "JumpHack");
	
	A64HookSafe(Offsets64::WaterSpeed,  (void *) WaterSpeed, (void **) &old_WaterSpeed, "WaterSpeed");
	
	A64HookSafe(Offsets64::LavaSpeed,  (void *) LavaSpeed, (void **) &old_LavaSpeed, "LavaSpeed");
	
	hexPatches.NoSlowDown = MemoryPatch::createWithHex(MineLib, Offsets64::NoSlowDown, "C0 03 5F D6");
	__android_log_print(hexPatches.NoSlowDown.isValid()?ANDROID_LOG_INFO:ANDROID_LOG_WARN, "ApollonOffset", "PATCH-%s %s @ 0x%lx", hexPatches.NoSlowDown.isValid()?"ok":"skip", "NoSlowDown", (unsigned long)Offsets64::NoSlowDown);
	
	// WaterDrown and Gravity resolve to the SAME body in 1.26.66.1
	// (MobMovementGravity::tickPlayerWaterGravity). Writing a `ret` here and then
	// installing an inline hook over the same address one statement later would
	// corrupt the trampoline (dobby patches the bytes it saved, which we already
	// overwrote). Let the Gravity hook own that body; skip the duplicate patch.
	if (Offsets64::WaterDrown != 0 && Offsets64::WaterDrown != Offsets64::Gravity) {
	    hexPatches.WaterDrown = MemoryPatch::createWithHex(MineLib, Offsets64::WaterDrown, "C0 03 5F D6");
	    __android_log_print(hexPatches.WaterDrown.isValid()?ANDROID_LOG_INFO:ANDROID_LOG_WARN, "ApollonOffset", "PATCH-%s %s @ 0x%lx", hexPatches.WaterDrown.isValid()?"ok":"skip", "WaterDrown", (unsigned long)Offsets64::WaterDrown);
	} else {
	    __android_log_print(ANDROID_LOG_INFO, "ApollonOffset", "PATCH-skip WaterDrown @ 0x%lx (aliases Gravity)", (unsigned long)Offsets64::WaterDrown);
	}
	
	hexPatches.LavaDrown = MemoryPatch::createWithHex(MineLib, Offsets64::LavaDrown, "C0 03 5F D6");
	__android_log_print(hexPatches.LavaDrown.isValid()?ANDROID_LOG_INFO:ANDROID_LOG_WARN, "ApollonOffset", "PATCH-%s %s @ 0x%lx", hexPatches.LavaDrown.isValid()?"ok":"skip", "LavaDrown", (unsigned long)Offsets64::LavaDrown);
	
	A64HookSafe(Offsets64::Gravity,  (void *) Gravity, (void **) &old_Gravity, "Gravity");
	
	hexPatches.Noclip = MemoryPatch::createWithHex(MineLib, Offsets64::Noclip, "C0 03 5F D6");
	__android_log_print(hexPatches.Noclip.isValid()?ANDROID_LOG_INFO:ANDROID_LOG_WARN, "ApollonOffset", "PATCH-%s %s @ 0x%lx", hexPatches.Noclip.isValid()?"ok":"skip", "Noclip", (unsigned long)Offsets64::Noclip);
	
	hexPatches.AutoClickMine = MemoryPatch::createWithHex(MineLib, Offsets64::AutoClickMine, "C0 03 5F D6");
	__android_log_print(hexPatches.AutoClickMine.isValid()?ANDROID_LOG_INFO:ANDROID_LOG_WARN, "ApollonOffset", "PATCH-%s %s @ 0x%lx", hexPatches.AutoClickMine.isValid()?"ok":"skip", "AutoClickMine", (unsigned long)Offsets64::AutoClickMine);
	
	A64HookSafe(Offsets64::FastBridge,  (void *) GameMode_FastBridge, (void **) &old_GameMode_FastBridge, "FastBridge");
	
	A64HookSafe(Offsets64::BlockReach,  (void *) GameMode_BlockReach, (void **) &old_GameMode_BlockReach, "BlockReach");
	
	hexPatches.XrayCameraThird = MemoryPatch::createWithHex(MineLib, Offsets64::XrayCameraThird, "C0 03 5F D6");
	__android_log_print(hexPatches.XrayCameraThird.isValid()?ANDROID_LOG_INFO:ANDROID_LOG_WARN, "ApollonOffset", "PATCH-%s %s @ 0x%lx", hexPatches.XrayCameraThird.isValid()?"ok":"skip", "XrayCameraThird", (unsigned long)Offsets64::XrayCameraThird);
	
	hexPatches.NoEmoteUseCooldown = MemoryPatch::createWithHex(MineLib, Offsets64::IsEmoting, "00 00 80 D2 C0 03 5F D6");
	__android_log_print(hexPatches.NoEmoteUseCooldown.isValid()?ANDROID_LOG_INFO:ANDROID_LOG_WARN, "ApollonOffset", "PATCH-%s %s @ 0x%lx", hexPatches.NoEmoteUseCooldown.isValid()?"ok":"skip", "IsEmoting", (unsigned long)Offsets64::IsEmoting);
	
	hexPatches.NoBlur1 = MemoryPatch::createWithHex(MineLib, Offsets64::NoBlur1, "C0 03 5F D6");
	__android_log_print(hexPatches.NoBlur1.isValid()?ANDROID_LOG_INFO:ANDROID_LOG_WARN, "ApollonOffset", "PATCH-%s %s @ 0x%lx", hexPatches.NoBlur1.isValid()?"ok":"skip", "NoBlur1", (unsigned long)Offsets64::NoBlur1);
	hexPatches.NoBlur2 = MemoryPatch::createWithHex(MineLib, Offsets64::NoBlur2, "C0 03 5F D6");
	__android_log_print(hexPatches.NoBlur2.isValid()?ANDROID_LOG_INFO:ANDROID_LOG_WARN, "ApollonOffset", "PATCH-%s %s @ 0x%lx", hexPatches.NoBlur2.isValid()?"ok":"skip", "NoBlur2", (unsigned long)Offsets64::NoBlur2);

#elif defined(__ARM_ARCH_7A__)

    DobbyHook(reinterpret_cast<void*>((const void*(*)(...))dlsym(handle, OBFUSCATE("Java_com_mojang_minecraftpe_MainActivity_nativeKeyHandler"))), reinterpret_cast<void*>(new_Java_com_mojang_minecraftpe_MainActivity_nativeKeyHandler), reinterpret_cast<void**>(&old_Java_com_mojang_minecraftpe_MainActivity_nativeKeyHandler));
	hexPatches.NativeKeyHandler = MemoryPatch::createWithHex(MineLib, Offsets::NativeKeyHandler, "70 47");
	hexPatches.NativeKeyHandler.Modify();
	
	MSHookThumb((void *)getAbsoluteAddress(MineLib, Offsets::NoNametags), (void *) BaseActorRenderer_renderText, (void **) &old_BaseActorRenderer_renderText);
	
	hexPatches.NoCaveVignette = MemoryPatch::createWithHex(MineLib, Offsets::VignetteRenderer, "70 47");
	
	hexPatches.AntiKnockBack = MemoryPatch::createWithHex(MineLib, Offsets::LerpMotion, "70 47");
	
	hexPatches.FullBright = MemoryPatch::createWithHex(MineLib, Offsets::FullBright, "00 20 C4 F2 B4 20 70 47");//return 90.0f
	MSHookThumb((void *)getAbsoluteAddress(MineLib, Offsets::Block_getRenderLayer), (void *) Block_getRenderLayer, (void **) &old_Block_getRenderLayer);
	
    MSHookThumb((void *)getAbsoluteAddress(MineLib, Offsets::ClientInstanceOnTick), (void *) ClientInstance_onTick, (void **) &old_ClientInstance_onTick);
	
    MSHookThumb((void *)getAbsoluteAddress(MineLib, Offsets::Subtitle), (void *) TitleMessage_setSubtitle, (void **) &old_TitleMessage_setSubtitle);
	
    MSHookThumb((void *)getAbsoluteAddress(MineLib, Offsets::ShowPaperdoll), (void *) HudPlayerRenderer_update, (void **) &old_HudPlayerRenderer_update);
	
    MSHookThumb((void *)getAbsoluteAddress(MineLib, Offsets::SetupFog), (void *) LevelRendererPlayer_setupFog, (void **) &old_LevelRendererPlayer_setupFog);
	
    MSHookThumb((void *)getAbsoluteAddress(MineLib, Offsets::GetMaxPlayers), (void *) AppPlatform_getMaxPlayers, (void **) &old_AppPlatform_getMaxPlayers);
	
	MSHookThumb((void *)getAbsoluteAddress(MineLib, Offsets::UnlockMarket), (void *) Entitlement_UnlockMarket, (void **) &old_Entitlement_UnlockMarket);
	
	MSHookThumb((void *)getAbsoluteAddress(MineLib, Offsets::InMainMenu), (void *) InMainMenu, (void **) &old_InMainMenu);
	MSHookThumb((void *)getAbsoluteAddress(MineLib, Offsets::InLanGame1), (void *) InLanGame1, (void **) &old_InLanGame1);
	MSHookThumb((void *)getAbsoluteAddress(MineLib, Offsets::InLanGame2), (void *) InLanGame2, (void **) &old_InLanGame2);
	
	MSHookThumb((void *)getAbsoluteAddress(MineLib, Offsets::Fov), (void *) PlayerFov, (void **) &old_PlayerFov);

	MSHookThumb((void *)getAbsoluteAddress(MineLib, Offsets::FogState), (void *) LevelRendererCamera_FogState, (void **) &old_LevelRendererCamera_FogState);

	hexPatches.Font0 = MemoryPatch::createWithHex(MineLib, Offsets::Font, "00 00 00 3F");//0
	hexPatches.Font1 = MemoryPatch::createWithHex(MineLib, Offsets::Font, "9A 99 19 3F");//1
	hexPatches.Font2 = MemoryPatch::createWithHex(MineLib, Offsets::Font, "33 33 33 3F");//2
	hexPatches.Font3 = MemoryPatch::createWithHex(MineLib, Offsets::Font, "CD CC 4C 3F");//3
	hexPatches.Font4 = MemoryPatch::createWithHex(MineLib, Offsets::Font, "66 66 66 3F");//4
	hexPatches.Font5 = MemoryPatch::createWithHex(MineLib, Offsets::Font, "00 00 80 3F");//5
	hexPatches.Font6 = MemoryPatch::createWithHex(MineLib, Offsets::Font, "CD CC 8C 3F");//6
	hexPatches.Font7 = MemoryPatch::createWithHex(MineLib, Offsets::Font, "9A 99 99 3F");//7
	hexPatches.Font8 = MemoryPatch::createWithHex(MineLib, Offsets::Font, "66 66 A6 3F");//8
	hexPatches.Font9 = MemoryPatch::createWithHex(MineLib, Offsets::Font, "33 33 B3 3F");//9
	hexPatches.Font10 = MemoryPatch::createWithHex(MineLib, Offsets::Font, "00 00 C0 3F");//10
	
	MSHookThumb((void *)getAbsoluteAddress(MineLib, Offsets::FastDrop), (void *) DropItemTime, (void **) &old_DropItemTime);
	
	hexPatches.NoHurtCam = MemoryPatch::createWithHex(MineLib, Offsets::NoHurtCam, "70 47");
	
	hexPatches.NoCamDistortion = MemoryPatch::createWithHex(MineLib, Offsets::NoCamDistortion, "70 47");
	
	hexPatches.NoBoatRotation = MemoryPatch::createWithHex(MineLib, Offsets::NoBoatRotation, "70 47");
	
	hexPatches.NoCamSleep = MemoryPatch::createWithHex(MineLib, Offsets::NoCamSleep, "70 47");
	
	hexPatches.PlaceCamera = MemoryPatch::createWithHex(MineLib, Offsets::PlaceCamera, "70 47");
	
	MSHookThumb((void *)getAbsoluteAddress(MineLib, Offsets::SpeedHack), (void *) SpeedHack, (void **) &old_SpeedHack);
	
	MSHookThumb((void *)getAbsoluteAddress(MineLib, Offsets::NormalTick), (void *) NormalTick, (void **) &old_NormalTick);
	MSHookThumb((void *)getAbsoluteAddress(MineLib, Offsets::Intersects), (void*) Intersects, (void**) &old_Intersects);
	MSHookThumb((void *)getAbsoluteAddress(MineLib, Offsets::PlayerAuthInput), (void*) PlayerAuthInputPacket, (void**) &old_PlayerAuthInputPacket);
	
	MSHookThumb((void *)getAbsoluteAddress(MineLib, Offsets::WaterTrigger1), (void *) WasInWater, (void **) &old_WasInWater);
	MSHookThumb((void *)getAbsoluteAddress(MineLib, Offsets::WaterTrigger2), (void *) ActorHeadInWater, (void **) &old_ActorHeadInWater);
	MSHookThumb((void *)getAbsoluteAddress(MineLib, Offsets::WaterTrigger3), (void *) WaterSplashEffect, (void **) &old_WaterSplashEffect);
	
	hexPatches.SlowDownTriggers = MemoryPatch::createWithHex(MineLib, Offsets::SlowDownTriggers, "70 47");
	
	MSHookThumb((void *)getAbsoluteAddress(MineLib, Offsets::JumpHack), (void *) JumpHack, (void **) &old_JumpHack);
	
	MSHookThumb((void *)getAbsoluteAddress(MineLib, Offsets::WaterSpeed), (void *) WaterSpeed, (void **) &old_WaterSpeed);
	
	MSHookThumb((void *)getAbsoluteAddress(MineLib, Offsets::LavaSpeed), (void *) LavaSpeed, (void **) &old_LavaSpeed);
	
	hexPatches.NoSlowDown = MemoryPatch::createWithHex(MineLib, Offsets::NoSlowDown, "70 47");
	
	hexPatches.WaterDrown = MemoryPatch::createWithHex(MineLib, Offsets::WaterDrown, "70 47");
	
	hexPatches.LavaDrown = MemoryPatch::createWithHex(MineLib, Offsets::LavaDrown, "70 47");
	
	MSHookThumb((void *)getAbsoluteAddress(MineLib, Offsets::Gravity), (void *) Gravity, (void **) &old_Gravity);
	
	hexPatches.Noclip = MemoryPatch::createWithHex(MineLib, Offsets::Noclip, "70 47");
	
	hexPatches.AutoClickMine = MemoryPatch::createWithHex(MineLib, Offsets::AutoClickMine, "70 47");
	
	MSHookThumb((void *)getAbsoluteAddress(MineLib, Offsets::FastBridge), (void *) GameMode_FastBridge, (void **) &old_GameMode_FastBridge);
	
	MSHookThumb((void *)getAbsoluteAddress(MineLib, Offsets::BlockReach), (void *) GameMode_BlockReach, (void **) &old_GameMode_BlockReach);
	
	hexPatches.XrayCameraThird = MemoryPatch::createWithHex(MineLib, Offsets::XrayCameraThird, "70 47");
	
	hexPatches.NoEmoteUseCooldown = MemoryPatch::createWithHex(MineLib, Offsets::IsEmoting, "00 20 70 47");
	
	hexPatches.NoBlur1 = MemoryPatch::createWithHex(MineLib, Offsets::NoBlur1, "70 47");
	hexPatches.NoBlur2 = MemoryPatch::createWithHex(MineLib, Offsets::NoBlur2, "70 47");
	
#else

    MSHookX86_64(reinterpret_cast<void*>((const void*(*)(...))dlsym(handle, OBFUSCATE("Java_com_mojang_minecraftpe_MainActivity_nativeKeyHandler"))), reinterpret_cast<void*>(new_Java_com_mojang_minecraftpe_MainActivity_nativeKeyHandler), reinterpret_cast<void**>(&old_Java_com_mojang_minecraftpe_MainActivity_nativeKeyHandler));
	hexPatches.NativeKeyHandler = MemoryPatch::createWithHex(MineLib, Offsets86_64::NativeKeyHandler, "C3");
	hexPatches.NativeKeyHandler.Modify();
	
	hexPatches.AntiKnockBack = MemoryPatch::createWithHex(MineLib, Offsets86_64::LerpMotion, "C3");
	
	hexPatches.FullBright = MemoryPatch::createWithHex(MineLib, Offsets86_64::FullBright, "B8 00 D0 77 44 66 0F 6E C0 C3");//return 999.0f
	hexPatches.SmoothLight = MemoryPatch::createWithHex(MineLib, Offsets86_64::SmoothLight, "31 C0 C3");//return 0
	MSHookX86_64((void *)getAbsoluteAddress(MineLib, Offsets86_64::Block_getRenderLayer), (void *) Block_getRenderLayer, (void **) &old_Block_getRenderLayer);
	
    MSHookX86_64((void *)getAbsoluteAddress(MineLib, Offsets86_64::ClientInstanceOnTick), (void *) ClientInstance_onTick, (void **) &old_ClientInstance_onTick);

    MSHookX86_64((void *)getAbsoluteAddress(MineLib, Offsets86_64::Subtitle), (void *) TitleMessage_setSubtitle, (void **) &old_TitleMessage_setSubtitle);
	
    MSHookX86_64((void *)getAbsoluteAddress(MineLib, Offsets86_64::ShowPaperdoll), (void *) HudPlayerRenderer_update, (void **) &old_HudPlayerRenderer_update);
	
    MSHookX86_64((void *)getAbsoluteAddress(MineLib, Offsets86_64::SetupFog), (void *) LevelRendererPlayer_setupFog, (void **) &old_LevelRendererPlayer_setupFog);
	
    MSHookX86_64((void *)getAbsoluteAddress(MineLib, Offsets86_64::GetMaxPlayers), (void *) AppPlatform_getMaxPlayers, (void **) &old_AppPlatform_getMaxPlayers);
	
    MSHookX86_64((void *)getAbsoluteAddress(MineLib, Offsets86_64::UnlockMarket), (void *) Entitlement_UnlockMarket, (void **) &old_Entitlement_UnlockMarket);
	
	MSHookX86_64((void *)getAbsoluteAddress(MineLib, Offsets86_64::InMainMenu), (void *) InMainMenu, (void **) &old_InMainMenu);
	MSHookX86_64((void *)getAbsoluteAddress(MineLib, Offsets86_64::InGame), (void *) InGame, (void **) &old_InGame);
	MSHookX86_64((void *)getAbsoluteAddress(MineLib, Offsets86_64::InLanGame1), (void *) InLanGame1, (void **) &old_InLanGame1);
	MSHookX86_64((void *)getAbsoluteAddress(MineLib, Offsets86_64::InLanGame2), (void *) InLanGame2, (void **) &old_InLanGame2);
	
	MSHookX86_64((void *)getAbsoluteAddress(MineLib, Offsets86_64::Fov), (void *) PlayerFov, (void **) &old_PlayerFov);

	MSHookX86_64((void *)getAbsoluteAddress(MineLib, Offsets86_64::FogState), (void *) LevelRendererCamera_FogState, (void **) &old_LevelRendererCamera_FogState);

	hexPatches.Font0 = MemoryPatch::createWithHex(MineLib, Offsets86_64::Font, "00 00 00 3F");//0
	hexPatches.Font1 = MemoryPatch::createWithHex(MineLib, Offsets86_64::Font, "9A 99 19 3F");//1
	hexPatches.Font2 = MemoryPatch::createWithHex(MineLib, Offsets86_64::Font, "33 33 33 3F");//2
	hexPatches.Font3 = MemoryPatch::createWithHex(MineLib, Offsets86_64::Font, "CD CC 4C 3F");//3
	hexPatches.Font4 = MemoryPatch::createWithHex(MineLib, Offsets86_64::Font, "66 66 66 3F");//4
	hexPatches.Font5 = MemoryPatch::createWithHex(MineLib, Offsets86_64::Font, "00 00 80 3F");//5
	hexPatches.Font6 = MemoryPatch::createWithHex(MineLib, Offsets86_64::Font, "CD CC 8C 3F");//6
	hexPatches.Font7 = MemoryPatch::createWithHex(MineLib, Offsets86_64::Font, "9A 99 99 3F");//7
	hexPatches.Font8 = MemoryPatch::createWithHex(MineLib, Offsets86_64::Font, "66 66 A6 3F");//8
	hexPatches.Font9 = MemoryPatch::createWithHex(MineLib, Offsets86_64::Font, "33 33 B3 3F");//9
	hexPatches.Font10 = MemoryPatch::createWithHex(MineLib, Offsets86_64::Font, "00 00 C0 3F");//10
	
	MSHookX86_64((void *)getAbsoluteAddress(MineLib, Offsets86_64::FastDrop), (void *) DropItemTime, (void **) &old_DropItemTime);
	
	hexPatches.NoHurtCam = MemoryPatch::createWithHex(MineLib, Offsets86_64::NoHurtCam, "C3");
	
	hexPatches.NoCamDistortion = MemoryPatch::createWithHex(MineLib, Offsets86_64::NoCamDistortion, "C3");
	
	hexPatches.NoBoatRotation = MemoryPatch::createWithHex(MineLib, Offsets86_64::NoBoatRotation, "C3");
	
	hexPatches.NoCamSleep = MemoryPatch::createWithHex(MineLib, Offsets86_64::NoCamSleep, "C3");
	
	hexPatches.PlaceCamera = MemoryPatch::createWithHex(MineLib, Offsets86_64::PlaceCamera, "C3");
	
	MSHookX86_64((void *)getAbsoluteAddress(MineLib, Offsets86_64::SpeedHack), (void *) SpeedHack, (void **) &old_SpeedHack);
	
	MSHookX86_64((void *)getAbsoluteAddress(MineLib, Offsets86_64::NormalTick), (void*) NormalTick, (void**) &old_NormalTick);
	MSHookX86_64((void *)getAbsoluteAddress(MineLib, Offsets86_64::Intersects), (void*) Intersects, (void**) &old_Intersects);
	MSHookX86_64((void *)getAbsoluteAddress(MineLib, Offsets86_64::PlayerAuthInput), (void *) PlayerAuthInputPacket, (void **) &old_PlayerAuthInputPacket);
	
	MSHookX86_64((void *)getAbsoluteAddress(MineLib, Offsets86_64::WaterTrigger1), (void *) WasInWater, (void **) &old_WasInWater);
	MSHookX86_64((void *)getAbsoluteAddress(MineLib, Offsets86_64::WaterTrigger2), (void *) ActorHeadInWater, (void **) &old_ActorHeadInWater);
	MSHookX86_64((void *)getAbsoluteAddress(MineLib, Offsets86_64::WaterTrigger3), (void *) WaterSplashEffect, (void **) &old_WaterSplashEffect);
	
	hexPatches.SlowDownTriggers = MemoryPatch::createWithHex(MineLib, Offsets86_64::SlowDownTriggers, "C3");
	
	MSHookX86_64((void *)getAbsoluteAddress(MineLib, Offsets86_64::JumpHack), (void *) JumpHack, (void **) &old_JumpHack);
	
	MSHookX86_64((void *)getAbsoluteAddress(MineLib, Offsets86_64::ElytraSpeed), (void *) ElytraSpeed, (void **) &old_ElytraSpeed);
	
	MSHookX86_64((void *)getAbsoluteAddress(MineLib, Offsets86_64::WaterSpeed), (void *) WaterSpeed, (void **) &old_WaterSpeed);
	
	MSHookX86_64((void *)getAbsoluteAddress(MineLib, Offsets86_64::LavaSpeed), (void *) LavaSpeed, (void **) &old_LavaSpeed);
	
	hexPatches.NoSlowDown = MemoryPatch::createWithHex(MineLib, Offsets86_64::NoSlowDown, "C3");
	
	hexPatches.WaterDrown = MemoryPatch::createWithHex(MineLib, Offsets86_64::WaterDrown, "C3");
	
	hexPatches.LavaDrown = MemoryPatch::createWithHex(MineLib, Offsets86_64::LavaDrown, "C3");
	
	MSHookX86_64((void *)getAbsoluteAddress(MineLib, Offsets86_64::BoatSpeed), (void *) BoatSpeed, (void **) &old_BoatSpeed);
	
	MSHookX86_64((void *)getAbsoluteAddress(MineLib, Offsets86_64::Gravity), (void *) Gravity, (void **) &old_Gravity);
	
	hexPatches.Noclip = MemoryPatch::createWithHex(MineLib, Offsets86_64::Noclip, "C3");
	
	hexPatches.AutoClickMine = MemoryPatch::createWithHex(MineLib, Offsets86_64::AutoClickMine, "C3");
	
	MSHookX86_64((void *)getAbsoluteAddress(MineLib, Offsets86_64::FastBridge), (void *) GameMode_FastBridge, (void **) &old_GameMode_FastBridge);
	
	MSHookX86_64((void *)getAbsoluteAddress(MineLib, Offsets86_64::BlockReach), (void *) GameMode_BlockReach, (void **) &old_GameMode_BlockReach);
	
	hexPatches.XrayCameraThird = MemoryPatch::createWithHex(MineLib, Offsets86_64::XrayCameraThird, "C3");
	
	hexPatches.NoEmoteUseCooldown = MemoryPatch::createWithHex(MineLib, Offsets86_64::IsEmoting, "48 C7 C0 00 00 00 00 C3");
	
	hexPatches.NoBlur1 = MemoryPatch::createWithHex(MineLib, Offsets86_64::NoBlur1, "C3");
	hexPatches.NoBlur2 = MemoryPatch::createWithHex(MineLib, Offsets86_64::NoBlur2, "C3");
	
#endif
	
    isHackLoaded = true;
    return NULL;
	} else {
		pthread_kill(cheatThread, SIGUSR1);
	}
}

// ---------------------------------------------------------------------------
//  LeviLauncher (preloader-android) native-mod lifecycle.
//
//  A native mod is dlopen()ed by the launcher and driven through the
//  PLGetModRegistration() dispatch table. JNI_OnLoad is NEVER called in that
//  path, so the previous build - which only implemented JNI_OnLoad - could
//  never start. These entry points make the mod start under LeviLauncher.
//
//  This build is "Levi native mod only" by request: JNI_OnLoad is removed so
//  the two paths cannot fight over cheatThread.
// ---------------------------------------------------------------------------
#include "modmenu.h"

extern "C" __attribute__((visibility("default")))
void ApollonNativeMod_Start() {
    if (cheatThread) return;                    // already running
    pthread_create(&cheatThread, NULL, hack_thread, NULL);

}

extern "C" __attribute__((visibility("default")))
void ApollonNativeMod_Stop() {
    if (cheatThread) {
        pthread_kill(cheatThread, SIGUSR1);
        cheatThread = 0;
    }
}

// Minimal, dependency-free registration ABI. Matches pl::mod::ModRegistration
// layout (instance + four bool(void*, void*) slots) without pulling in the
// preloader headers or std::filesystem, which keeps this .so buildable with
// the existing bionic sysroot.
//
// PHASE RULES (learned from preloader-android's source, not guessed):
//
//  * ONLY `load` runs inside pl::runtime::ScopedModMenuOwner, which sets the
//    "owning mod" that registerModule() checks against. Registering a module
//    from enable() or from a background thread leaves that owner empty and the
//    launcher logs "Rejected Mod Menu module ... does not match owning
//    lifecycle mod". So the Mod Menu registration MUST happen here in `load`.
//
//  * `load` owns no game hooks. Hooks need libminecraftpe.so to be present and
//    are installed on the cheat thread, which `enable` starts. Keeping them
//    separate means a failed hook can never stop the menu from appearing.
//
//  * Returning false from `load` makes the launcher drop the mod entirely, so
//    the menu registration result is logged but never fatal.
extern "C" __attribute__((visibility("default")))
void *PLGetModRegistration() {
    struct Reg {
        void *instance;
        bool (*load)(void *, void *);
        bool (*enable)(void *, void *);
        bool (*disable)(void *, void *);
        bool (*unload)(void *, void *);
    };
    static Reg reg = {
        nullptr,
        // load: runs under ScopedModMenuOwner -> this is where the menu registers.
        +[](void *, void *) -> bool {
            const int n = ApollonRegisterModMenu();
            __android_log_print(n > 0 ? ANDROID_LOG_INFO : ANDROID_LOG_ERROR,
                                "ApollonMenu",
                                "load phase: registered %d modules "
                                "(0 = menu API unreachable or all rejected)", n);
            return true;   // never fail the load over the menu
        },
        // enable: start hooks.
        +[](void *, void *) -> bool { ApollonNativeMod_Start(); return true; },
        // disable: leave hooks installed but stop the thread.
        +[](void *, void *) -> bool { return true; },
        // unload: stop.
        +[](void *, void *) -> bool { ApollonNativeMod_Stop();  return true; },
    };
    return &reg;
}

// JNI_OnLoad/JNI_OnUnload removed: this build is driven exclusively by the
// LeviLauncher preloader lifecycle above (see PLGetModRegistration).

