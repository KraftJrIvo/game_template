#include "game_basic.h"
#include "raylib.h"
#include "rlgl.h"

#include "util/vec_ops.h"
#include <string>

#include "resources.h"

void _loadAssets(GameAssets& ga, GameState& gs) 
{
    ga.sprite = LoadTextureFromImage(LoadImageFromMemory(".png", res_sprite_png, res_sprite_png_len));
    ga.music = LoadMusicStreamFromMemory(".ogg", res_music_ogg, res_music_ogg_len);
    ga.sound = LoadSoundFromWave(LoadWaveFromMemory(".ogg", res_sound_ogg, res_sound_ogg_len));
    ga.postProcFragShader = LoadShaderFromMemory(NULL, (const char*)prepShader((unsigned char*)res_post_proc_fs).c_str());
    
    const char8_t allChars[228] = u8" !\"#$%&\'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~абвгдеёжзийклмнопрстуфхцчшщъыьэюяАБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ";
    int c; auto cdpts = LoadCodepoints((const char*)allChars, &c);
    ga.font = LoadFontFromMemory(".otf", res_font_otf, res_font_otf_len, 39, cdpts, c);

    ga.spriteSz = Vector2{100.0f, 100.0f};
}

void _reset(GameState& gs) {
    gs.spritePos = Vector2{float(GetScreenWidth()), float(GetScreenHeight())} * 0.5f;
    gs.spriteVel = RAND_FLOAT_SIGNED_2D;
}

void updateMusic(GameState& gs) {
    if (gs.usr.musEnabled) {
        UpdateMusicStream(gs.ga.p->music);
        auto musicLen = GetMusicTimeLength(gs.ga.p->music);
        auto musicTimePlayed = GetMusicTimePlayed(gs.ga.p->music);
    }
}

void _updateAndDraw(GameState& gs) {
    auto delta = GetFrameTime();
    if (delta < 0.1f) {
        gs.spritePos += gs.spriteVel * SPRITE_SPEED * GetFrameTime();
        if (gs.spritePos.x - gs.ga.p->spriteSz.x * 0.5f < 0 || gs.spritePos.x + gs.ga.p->spriteSz.x * 0.5f > GetScreenWidth()) {
            PlaySound(gs.ga.p->sound);
            gs.spriteVel.x *= -1.0f;
        }
        if (gs.spritePos.y - gs.ga.p->spriteSz.y * 0.5f < 0 || gs.spritePos.y + gs.ga.p->spriteSz.y * 0.5f > GetScreenHeight()) {
            PlaySound(gs.ga.p->sound);
            gs.spriteVel.y *= -1.0f;
        }
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        gs.usr.spriteColor = (gs.usr.spriteColor + 1) % COLORS.size();
        saveUserData(gs);
    }
    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
        gs.usr.musEnabled = !gs.usr.musEnabled;
        saveUserData(gs);
    }

    updateMusic(gs);
    
    ClearBackground(BLACK);
    DrawTextureV(gs.ga.p->sprite, gs.spritePos - gs.ga.p->spriteSz * 0.5f, COLORS[gs.usr.spriteColor]);
    DrawTextEx(gs.ga.p->font, std::to_string(gs.time).c_str(), {10.0f, 10.0f}, 39, 0, WHITE);
}