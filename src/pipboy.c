#include "pipboy.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "game/automap.h"
#include "color.h"
#include "game/combat.h"
#include "game/config.h"
#include "core.h"
#include "game/critter.h"
#include "game/cycle.h"
#include "game/bmpdlog.h"
#include "debug.h"
#include "draw.h"
#include "game/game.h"
#include "game/gconfig.h"
#include "game/gmouse.h"
#include "game/gmovie.h"
#include "game/gsound.h"
#include "game/intface.h"
#include "game/map.h"
#include "memory.h"
#include "game/object.h"
#include "queue.h"
#include "random.h"
#include "scripts.h"
#include "stat.h"
#include "text_font.h"
#include "window_manager.h"
#include "word_wrap.h"
#include "worldmap.h"

#define PIPBOY_RAND_MAX (32767)

// 0x496FC0
const Rect gPipboyWindowContentRect = {
    PIPBOY_WINDOW_CONTENT_VIEW_X,
    PIPBOY_WINDOW_CONTENT_VIEW_Y,
    PIPBOY_WINDOW_CONTENT_VIEW_X + PIPBOY_WINDOW_CONTENT_VIEW_WIDTH,
    PIPBOY_WINDOW_CONTENT_VIEW_Y + PIPBOY_WINDOW_CONTENT_VIEW_HEIGHT,
};

// 0x496FD0
const int gPipboyFrmIds[PIPBOY_FRM_COUNT] = {
    8,
    9,
    82,
    127,
    128,
    129,
    130,
    131,
    132,
    133,
    226,
};

// 0x51C128
QuestDescription* gQuestDescriptions = NULL;

// 0x51C12C
int gQuestsCount = 0;

// 0x51C130
HolodiskDescription* gHolodiskDescriptions = NULL;

// 0x51C134
int gHolodisksCount = 0;

// Number of rest options available.
//
// 0x51C138
int gPipboyRestOptionsCount = PIPBOY_REST_DURATION_COUNT;

// 0x51C13C
bool gPipboyWindowIsoWasEnabled = false;

// 0x51C140
const HolidayDescription gHolidayDescriptions[HOLIDAY_COUNT] = {
    { 1, 1, 100 },
    { 2, 14, 101 },
    { 4, 1, 102 },
    { 7, 4, 104 },
    { 10, 6, 103 },
    { 10, 31, 105 },
    { 11, 28, 106 },
    { 12, 25, 107 },
};

// 0x51C170
PipboyRenderProc* _PipFnctn[5] = {
    pipboyWindowHandleStatus,
    pipboyWindowHandleAutomaps,
    pipboyHandleVideoArchive,
    pipboyHandleAlarmClock,
    pipboyHandleAlarmClock,
};

// 0x6642E0
Size gPipboyFrmSizes[PIPBOY_FRM_COUNT];

// 0x664338
MessageListItem gPipboyMessageListItem;

// pipboy.msg
//
// 0x664348
MessageList gPipboyMessageList;

// 0x664350
STRUCT_664350 _sortlist[24];

// quests.msg
//
// 0x664410
MessageList gQuestsMessageList;

// 0x664418
int gPipboyQuestLocationsCount;

// 0x66441C
unsigned char* gPipboyWindowBuffer;

// 0x664420
unsigned char* gPipboyFrmData[PIPBOY_FRM_COUNT];

// 0x66444C
int gPipboyWindowHolodisksCount;

// 0x664450
int gPipboyMouseY;

// 0x664454
int gPipboyMouseX;

// 0x664458
unsigned int gPipboyLastEventTimestamp;

// Index of the last page when rendering holodisk content.
//
// 0x66445C
int gPipboyHolodiskLastPage;

// 0x664460
int _HotLines[22];

// 0x6644B8
int _button;

// 0x6644BC
int gPipboyPreviousMouseX;

// 0x6644C0
int gPipboyPreviousMouseY;

// 0x6644C4
int gPipboyWindow;

// 0x6644C8
CacheEntry* gPipboyFrmHandles[PIPBOY_FRM_COUNT];

int _holodisk;

// 0x6644F8
int gPipboyWindowButtonCount;

// 0x6644FC
int gPipboyWindowOldFont;

// 0x664500
bool _proc_bail_flag;

// 0x664504
int _amlst_mode;

// 0x664508
int gPipboyTab;

// 0x66450C
int _actcnt;

// 0x664510
int gPipboyWindowButtonStart;

// 0x664514
int gPipboyCurrentLine;

// 0x664518
int _rest_time;

// 0x66451C
int _amcty_indx;

// 0x664520
int _view_page;

// 0x664524
int gPipboyLinesCount;

// 0x664528
unsigned char _hot_back_line;

// 0x664529
unsigned char _holo_flag;

// 0x66452A
unsigned char _stat_flag;

// 0x497004
int pipboyOpen(int intent)
{
    if (!wmMapPipboyActive()) {
        // You aren't wearing the pipboy!
        const char* text = getmsg(&misc_message_file, &gPipboyMessageListItem, 7000);
        dialog_out(text, NULL, 0, 192, 135, colorTable[32328], NULL, colorTable[32328], 1);
        return 0;
    }

    intent = pipboyWindowInit(intent);
    if (intent == -1) {
        return -1;
    }

    mouse_get_position(&gPipboyPreviousMouseX, &gPipboyPreviousMouseY);
    gPipboyLastEventTimestamp = _get_time();

    while (true) {
        int keyCode = _get_input();

        if (intent == PIPBOY_OPEN_INTENT_REST) {
            keyCode = 504;
            intent = PIPBOY_OPEN_INTENT_UNSPECIFIED;
        }

        mouse_get_position(&gPipboyMouseX, &gPipboyMouseY);

        if (keyCode != -1 || gPipboyMouseX != gPipboyPreviousMouseX || gPipboyMouseY != gPipboyPreviousMouseY) {
            gPipboyLastEventTimestamp = _get_time();
            gPipboyPreviousMouseX = gPipboyMouseX;
            gPipboyPreviousMouseY = gPipboyMouseY;
        } else {
            if (_get_time() - gPipboyLastEventTimestamp > PIPBOY_IDLE_TIMEOUT) {
                pipboyRenderScreensaver();

                gPipboyLastEventTimestamp = _get_time();
                mouse_get_position(&gPipboyPreviousMouseX, &gPipboyPreviousMouseY);
            }
        }

        if (keyCode == KEY_CTRL_Q || keyCode == KEY_CTRL_X || keyCode == KEY_F10) {
            game_quit_with_confirm();
            break;
        }

        if (keyCode == 503 || keyCode == KEY_ESCAPE || keyCode == KEY_RETURN || keyCode == KEY_UPPERCASE_P || keyCode == KEY_LOWERCASE_P || game_user_wants_to_quit != 0) {
            break;
        }

        if (keyCode == KEY_F12) {
            takeScreenshot();
        } else if (keyCode >= 500 && keyCode <= 504) {
            gPipboyTab = keyCode - 500;
            _PipFnctn[gPipboyTab](1024);
        } else if (keyCode >= 505 && keyCode <= 527) {
            _PipFnctn[gPipboyTab](keyCode - 506);
        } else if (keyCode == 528) {
            _PipFnctn[gPipboyTab](1025);
        } else if (keyCode == KEY_PAGE_DOWN) {
            _PipFnctn[gPipboyTab](1026);
        } else if (keyCode == KEY_PAGE_UP) {
            _PipFnctn[gPipboyTab](1027);
        }

        if (_proc_bail_flag) {
            break;
        }
    }

    pipboyWindowFree();

    return 0;
}

// 0x497228
int pipboyWindowInit(int intent)
{
    gPipboyWindowIsoWasEnabled = map_disable_bk_processes();

    cycle_disable();
    gmouse_3d_off();
    disable_box_bar_win();
    gmouse_set_cursor(MOUSE_CURSOR_ARROW);

    gPipboyRestOptionsCount = PIPBOY_REST_DURATION_COUNT_WITHOUT_PARTY;

    if (getPartyMemberCount() > 1 && partyMemberNeedsHealing()) {
        gPipboyRestOptionsCount = PIPBOY_REST_DURATION_COUNT;
    }

    gPipboyWindowOldFont = fontGetCurrent();
    fontSetCurrent(101);

    _proc_bail_flag = 0;
    _rest_time = 0;
    gPipboyCurrentLine = 0;
    gPipboyWindowButtonCount = 0;
    gPipboyLinesCount = PIPBOY_WINDOW_CONTENT_VIEW_HEIGHT / fontGetLineHeight() - 1;
    gPipboyWindowButtonStart = 0;
    _hot_back_line = 0;

    if (holodiskInit() == -1) {
        return -1;
    }

    if (!message_init(&gPipboyMessageList)) {
        return -1;
    }

    char path[MAX_PATH];
    sprintf(path, "%s%s", msg_path, "pipboy.msg");

    if (!(message_load(&gPipboyMessageList, path))) {
        return -1;
    }

    int index;
    for (index = 0; index < PIPBOY_FRM_COUNT; index++) {
        int fid = art_id(OBJ_TYPE_INTERFACE, gPipboyFrmIds[index], 0, 0, 0);
        gPipboyFrmData[index] = art_lock(fid, &(gPipboyFrmHandles[index]), &(gPipboyFrmSizes[index].width), &(gPipboyFrmSizes[index].height));
        if (gPipboyFrmData[index] == NULL) {
            break;
        }
    }

    if (index != PIPBOY_FRM_COUNT) {
        debugPrint("\n** Error loading pipboy graphics! **\n");

        while (--index >= 0) {
            art_ptr_unlock(gPipboyFrmHandles[index]);
        }

        return -1;
    }

    int pipboyWindowX = 0;
    int pipboyWindowY = 0;
    gPipboyWindow = windowCreate(pipboyWindowX, pipboyWindowY, PIPBOY_WINDOW_WIDTH, PIPBOY_WINDOW_HEIGHT, colorTable[0], WINDOW_FLAG_0x10);
    if (gPipboyWindow == -1) {
        debugPrint("\n** Error opening pipboy window! **\n");
        for (int index = 0; index < PIPBOY_FRM_COUNT; index++) {
            art_ptr_unlock(gPipboyFrmHandles[index]);
        }
        return -1;
    }

    gPipboyWindowBuffer = windowGetBuffer(gPipboyWindow);
    memcpy(gPipboyWindowBuffer, gPipboyFrmData[PIPBOY_FRM_BACKGROUND], PIPBOY_WINDOW_WIDTH * PIPBOY_WINDOW_HEIGHT);

    pipboyDrawNumber(gameTimeGetHour(), 4, PIPBOY_WINDOW_TIME_X, PIPBOY_WINDOW_TIME_Y);
    pipboyDrawDate();

    int alarmButton = buttonCreate(gPipboyWindow,
        124,
        13,
        gPipboyFrmSizes[PIPBOY_FRM_ALARM_UP].width,
        gPipboyFrmSizes[PIPBOY_FRM_ALARM_UP].height,
        -1,
        -1,
        -1,
        504,
        gPipboyFrmData[PIPBOY_FRM_ALARM_UP],
        gPipboyFrmData[PIPBOY_FRM_ALARM_DOWN],
        NULL,
        BUTTON_FLAG_TRANSPARENT);
    if (alarmButton != -1) {
        buttonSetCallbacks(alarmButton, gsound_med_butt_press, gsound_med_butt_release);
    }

    int y = 341;
    int eventCode = 500;
    for (int index = 0; index < 5; index += 1) {
        if (index != 1) {
            int btn = buttonCreate(gPipboyWindow,
                53,
                y,
                gPipboyFrmSizes[PIPBOY_FRM_LITTLE_RED_BUTTON_UP].width,
                gPipboyFrmSizes[PIPBOY_FRM_LITTLE_RED_BUTTON_UP].height,
                -1,
                -1,
                -1,
                eventCode,
                gPipboyFrmData[PIPBOY_FRM_LITTLE_RED_BUTTON_UP],
                gPipboyFrmData[PIPBOY_FRM_LITTLE_RED_BUTTON_DOWN],
                NULL,
                BUTTON_FLAG_TRANSPARENT);
            if (btn != -1) {
                buttonSetCallbacks(btn, gsound_red_butt_press, gsound_red_butt_release);
            }

            eventCode += 1;
        }

        y += 27;
    }

    if (intent == PIPBOY_OPEN_INTENT_REST) {
        if (!critter_can_obj_dude_rest()) {
            blitBufferToBufferTrans(
                gPipboyFrmData[PIPBOY_FRM_LOGO],
                gPipboyFrmSizes[PIPBOY_FRM_LOGO].width,
                gPipboyFrmSizes[PIPBOY_FRM_LOGO].height,
                gPipboyFrmSizes[PIPBOY_FRM_LOGO].width,
                gPipboyWindowBuffer + PIPBOY_WINDOW_WIDTH * 156 + 323,
                PIPBOY_WINDOW_WIDTH);

            int month;
            int day;
            int year;
            gameTimeGetDate(&month, &day, &year);

            int holiday = 0;
            for (; holiday < HOLIDAY_COUNT; holiday += 1) {
                const HolidayDescription* holidayDescription = &(gHolidayDescriptions[holiday]);
                if (holidayDescription->month == month && holidayDescription->day == day) {
                    break;
                }
            }

            if (holiday != HOLIDAY_COUNT) {
                const HolidayDescription* holidayDescription = &(gHolidayDescriptions[holiday]);
                const char* holidayName = getmsg(&gPipboyMessageList, &gPipboyMessageListItem, holidayDescription->textId);
                char holidayNameCopy[256];
                strcpy(holidayNameCopy, holidayName);

                int len = fontGetStringWidth(holidayNameCopy);
                fontDrawText(gPipboyWindowBuffer + PIPBOY_WINDOW_WIDTH * (gPipboyFrmSizes[PIPBOY_FRM_LOGO].height + 174) + 6 + gPipboyFrmSizes[PIPBOY_FRM_LOGO].width / 2 + 323 - len / 2,
                    holidayNameCopy,
                    350,
                    PIPBOY_WINDOW_WIDTH,
                    colorTable[992]);
            }

            win_draw(gPipboyWindow);

            gsound_play_sfx_file("iisxxxx1");

            const char* text = getmsg(&gPipboyMessageList, &gPipboyMessageListItem, 215);
            dialog_out(text, NULL, 0, 192, 135, colorTable[32328], 0, colorTable[32328], DIALOG_BOX_LARGE);

            intent = PIPBOY_OPEN_INTENT_UNSPECIFIED;
        }
    } else {
        blitBufferToBufferTrans(
            gPipboyFrmData[PIPBOY_FRM_LOGO],
            gPipboyFrmSizes[PIPBOY_FRM_LOGO].width,
            gPipboyFrmSizes[PIPBOY_FRM_LOGO].height,
            gPipboyFrmSizes[PIPBOY_FRM_LOGO].width,
            gPipboyWindowBuffer + PIPBOY_WINDOW_WIDTH * 156 + 323,
            PIPBOY_WINDOW_WIDTH);

        int month;
        int day;
        int year;
        gameTimeGetDate(&month, &day, &year);

        int holiday;
        for (holiday = 0; holiday < HOLIDAY_COUNT; holiday += 1) {
            const HolidayDescription* holidayDescription = &(gHolidayDescriptions[holiday]);
            if (holidayDescription->month == month && holidayDescription->day == day) {
                break;
            }
        }

        if (holiday != HOLIDAY_COUNT) {
            const HolidayDescription* holidayDescription = &(gHolidayDescriptions[holiday]);
            const char* holidayName = getmsg(&gPipboyMessageList, &gPipboyMessageListItem, holidayDescription->textId);
            char holidayNameCopy[256];
            strcpy(holidayNameCopy, holidayName);

            int length = fontGetStringWidth(holidayNameCopy);
            fontDrawText(gPipboyWindowBuffer + PIPBOY_WINDOW_WIDTH * (gPipboyFrmSizes[PIPBOY_FRM_LOGO].height + 174) + 6 + gPipboyFrmSizes[PIPBOY_FRM_LOGO].width / 2 + 323 - length / 2,
                holidayNameCopy,
                350,
                PIPBOY_WINDOW_WIDTH,
                colorTable[992]);
        }

        win_draw(gPipboyWindow);
    }

    if (questInit() == -1) {
        return -1;
    }

    gsound_play_sfx_file("pipon");
    win_draw(gPipboyWindow);

    return intent;
}

// 0x497828
void pipboyWindowFree()
{
    bool showScriptMessages = false;
    configGetBool(&game_config, GAME_CONFIG_DEBUG_KEY, GAME_CONFIG_SHOW_SCRIPT_MESSAGES_KEY, &showScriptMessages);

    if (showScriptMessages) {
        debugPrint("\nScript <Map Update>");
    }

    scriptsExecMapUpdateProc();

    windowDestroy(gPipboyWindow);

    message_exit(&gPipboyMessageList);

    // NOTE: Uninline.
    holodiskFree();

    for (int index = 0; index < PIPBOY_FRM_COUNT; index++) {
        art_ptr_unlock(gPipboyFrmHandles[index]);
    }

    pipboyWindowDestroyButtons();

    fontSetCurrent(gPipboyWindowOldFont);

    if (gPipboyWindowIsoWasEnabled) {
        map_enable_bk_processes();
    }

    cycle_enable();
    enable_box_bar_win();
    gmouse_set_cursor(MOUSE_CURSOR_ARROW);
    intface_redraw();

    // NOTE: Uninline.
    questFree();
}

// NOTE: Collapsed.
//
// 0x497918
void _pip_init_()
{
}

// NOTE: Uncollapsed 0x497918.
//
// pip_init
void pipboyInit()
{
    _pip_init_();
}

// NOTE: Uncollapsed 0x497918.
void pipboyReset()
{
    _pip_init_();
}

// 0x49791C
void pipboyDrawNumber(int value, int digits, int x, int y)
{
    int offset = PIPBOY_WINDOW_WIDTH * y + x + 9 * (digits - 1);

    for (int index = 0; index < digits; index++) {
        blitBufferToBuffer(gPipboyFrmData[PIPBOY_FRM_NUMBERS] + 9 * (value % 10), 9, 17, 360, gPipboyWindowBuffer + offset, PIPBOY_WINDOW_WIDTH);
        offset -= 9;
        value /= 10;
    }
}

// 0x4979B4
void pipboyDrawDate()
{
    int day;
    int month;
    int year;

    gameTimeGetDate(&month, &day, &year);
    pipboyDrawNumber(day, 2, PIPBOY_WINDOW_DAY_X, PIPBOY_WINDOW_DAY_Y);

    blitBufferToBuffer(gPipboyFrmData[PIPBOY_FRM_MONTHS] + 435 * (month - 1), 29, 14, 29, gPipboyWindowBuffer + PIPBOY_WINDOW_WIDTH * PIPBOY_WINDOW_MONTH_Y + PIPBOY_WINDOW_MONTH_X, PIPBOY_WINDOW_WIDTH);

    pipboyDrawNumber(year, 4, PIPBOY_WINDOW_YEAR_X, PIPBOY_WINDOW_YEAR_Y);
}

// 0x497A40
void pipboyDrawText(const char* text, int flags, int color)
{
    if ((flags & PIPBOY_TEXT_STYLE_UNDERLINE) != 0) {
        color |= FONT_UNDERLINE;
    }

    int left = 8;
    if ((flags & PIPBOY_TEXT_NO_INDENT) != 0) {
        left -= 7;
    }

    int length = fontGetStringWidth(text);

    if ((flags & PIPBOY_TEXT_ALIGNMENT_CENTER) != 0) {
        left = (350 - length) / 2;
    } else if ((flags & PIPBOY_TEXT_ALIGNMENT_RIGHT_COLUMN) != 0) {
        left += 175;
    } else if ((flags & PIPBOY_TEXT_ALIGNMENT_LEFT_COLUMN_CENTER) != 0) {
        left += 86 - length + 16;
    } else if ((flags & PIPBOY_TEXT_ALIGNMENT_RIGHT_COLUMN_CENTER) != 0) {
        left += 260 - length;
    }

    fontDrawText(gPipboyWindowBuffer + PIPBOY_WINDOW_WIDTH * (gPipboyCurrentLine * fontGetLineHeight() + PIPBOY_WINDOW_CONTENT_VIEW_Y) + PIPBOY_WINDOW_CONTENT_VIEW_X + left, text, PIPBOY_WINDOW_WIDTH, PIPBOY_WINDOW_WIDTH, color);

    if ((flags & PIPBOY_TEXT_STYLE_STRIKE_THROUGH) != 0) {
        int top = gPipboyCurrentLine * fontGetLineHeight() + 49;
        bufferDrawLine(gPipboyWindowBuffer, PIPBOY_WINDOW_WIDTH, PIPBOY_WINDOW_CONTENT_VIEW_X + left, top, PIPBOY_WINDOW_CONTENT_VIEW_X + left + length, top, color);
    }

    if (gPipboyCurrentLine < gPipboyLinesCount) {
        gPipboyCurrentLine += 1;
    }
}

// 0x497B64
void pipboyDrawBackButton(int color)
{
    if (gPipboyLinesCount >= 0) {
        gPipboyCurrentLine = gPipboyLinesCount;
    }

    blitBufferToBuffer(gPipboyFrmData[PIPBOY_FRM_BACKGROUND] + PIPBOY_WINDOW_WIDTH * 436 + 254, 350, 20, PIPBOY_WINDOW_WIDTH, gPipboyWindowBuffer + PIPBOY_WINDOW_WIDTH * 436 + 254, PIPBOY_WINDOW_WIDTH);

    // BACK
    const char* text = getmsg(&gPipboyMessageList, &gPipboyMessageListItem, 201);
    pipboyDrawText(text, PIPBOY_TEXT_ALIGNMENT_CENTER, color);
}

// NOTE: Collapsed.
//
// 0x497BD4
int _save_pipboy(File* stream)
{
    return 0;
}

// NOTE: Uncollapsed 0x497BD4.
int pipboySave(File* stream)
{
    return _save_pipboy(stream);
}

// NOTE: Uncollapsed 0x497BD4.
int pipboyLoad(File* stream)
{
    return _save_pipboy(stream);
}

// 0x497BD8
void pipboyWindowHandleStatus(int a1)
{
    if (a1 == 1024) {
        pipboyWindowDestroyButtons();
        blitBufferToBuffer(gPipboyFrmData[PIPBOY_FRM_BACKGROUND] + PIPBOY_WINDOW_WIDTH * PIPBOY_WINDOW_CONTENT_VIEW_Y + PIPBOY_WINDOW_CONTENT_VIEW_X,
            PIPBOY_WINDOW_CONTENT_VIEW_WIDTH,
            PIPBOY_WINDOW_CONTENT_VIEW_HEIGHT,
            PIPBOY_WINDOW_WIDTH,
            gPipboyWindowBuffer + PIPBOY_WINDOW_WIDTH * PIPBOY_WINDOW_CONTENT_VIEW_Y + PIPBOY_WINDOW_CONTENT_VIEW_X,
            PIPBOY_WINDOW_WIDTH);
        if (gPipboyLinesCount >= 0) {
            gPipboyCurrentLine = 0;
        }

        _holo_flag = 0;
        _holodisk = -1;
        gPipboyWindowHolodisksCount = 0;
        _view_page = 0;
        _stat_flag = 0;

        for (int index = 0; index < gHolodisksCount; index += 1) {
            HolodiskDescription* holodiskDescription = &(gHolodiskDescriptions[index]);
            if (game_global_vars[holodiskDescription->gvar] != 0) {
                gPipboyWindowHolodisksCount += 1;
                break;
            }
        }

        pipboyWindowRenderQuestLocationList(-1);

        if (gPipboyQuestLocationsCount == 0) {
            const char* text = getmsg(&gPipboyMessageList, &gPipboyMessageListItem, 203);
            pipboyDrawText(text, 0, colorTable[992]);
        }

        gPipboyWindowHolodisksCount = pipboyWindowRenderHolodiskList(-1);

        win_draw_rect(gPipboyWindow, &gPipboyWindowContentRect);
        pipboyWindowCreateButtons(2, gPipboyQuestLocationsCount + gPipboyWindowHolodisksCount + 1, false);
        win_draw(gPipboyWindow);
        return;
    }

    if (_stat_flag == 0 && _holo_flag == 0) {
        if (gPipboyQuestLocationsCount != 0 && gPipboyMouseX < 429) {
            gsound_play_sfx_file("ib1p1xx1");
            blitBufferToBuffer(gPipboyFrmData[PIPBOY_FRM_BACKGROUND] + PIPBOY_WINDOW_WIDTH * PIPBOY_WINDOW_CONTENT_VIEW_Y + PIPBOY_WINDOW_CONTENT_VIEW_X,
                PIPBOY_WINDOW_CONTENT_VIEW_WIDTH,
                PIPBOY_WINDOW_CONTENT_VIEW_HEIGHT,
                PIPBOY_WINDOW_WIDTH,
                gPipboyWindowBuffer + PIPBOY_WINDOW_WIDTH * PIPBOY_WINDOW_CONTENT_VIEW_Y + PIPBOY_WINDOW_CONTENT_VIEW_X,
                PIPBOY_WINDOW_WIDTH);
            pipboyWindowRenderQuestLocationList(a1);
            pipboyWindowRenderHolodiskList(-1);
            win_draw_rect(gPipboyWindow, &gPipboyWindowContentRect);
            coreDelayProcessingEvents(200);
            _stat_flag = 1;
        } else {
            if (gPipboyWindowHolodisksCount != 0 && gPipboyWindowHolodisksCount >= a1 && gPipboyMouseX > 429) {
                gsound_play_sfx_file("ib1p1xx1");
                _holodisk = 0;

                int index = 0;
                for (; index < gHolodisksCount; index += 1) {
                    HolodiskDescription* holodiskDescription = &(gHolodiskDescriptions[index]);
                    if (game_global_vars[holodiskDescription->gvar] > 0) {
                        if (a1 - 1 == _holodisk) {
                            break;
                        }
                        _holodisk += 1;
                    }
                }
                _holodisk = index;

                blitBufferToBuffer(gPipboyFrmData[PIPBOY_FRM_BACKGROUND] + PIPBOY_WINDOW_WIDTH * PIPBOY_WINDOW_CONTENT_VIEW_Y + PIPBOY_WINDOW_CONTENT_VIEW_X,
                    PIPBOY_WINDOW_CONTENT_VIEW_WIDTH,
                    PIPBOY_WINDOW_CONTENT_VIEW_HEIGHT,
                    PIPBOY_WINDOW_WIDTH,
                    gPipboyWindowBuffer + PIPBOY_WINDOW_WIDTH * PIPBOY_WINDOW_CONTENT_VIEW_Y + PIPBOY_WINDOW_CONTENT_VIEW_X,
                    PIPBOY_WINDOW_WIDTH);
                pipboyWindowRenderHolodiskList(_holodisk);
                pipboyWindowRenderQuestLocationList(-1);
                win_draw_rect(gPipboyWindow, &gPipboyWindowContentRect);
                coreDelayProcessingEvents(200);
                pipboyWindowDestroyButtons();
                pipboyRenderHolodiskText();
                pipboyWindowCreateButtons(0, 0, true);
                _holo_flag = 1;
            }
        }
    }

    if (_stat_flag == 0) {
        if (_holo_flag == 0 || a1 < 1025 || a1 > 1027) {
            return;
        }

        if ((gPipboyMouseX > 459 && a1 != 1027) || a1 == 1026) {
            if (gPipboyHolodiskLastPage <= _view_page) {
                if (a1 != 1026) {
                    gsound_play_sfx_file("ib1p1xx1");
                    blitBufferToBuffer(gPipboyFrmData[PIPBOY_FRM_BACKGROUND] + PIPBOY_WINDOW_WIDTH * 436 + 254, 350, 20, PIPBOY_WINDOW_WIDTH, gPipboyWindowBuffer + PIPBOY_WINDOW_WIDTH * 436 + 254, PIPBOY_WINDOW_WIDTH);

                    if (gPipboyLinesCount >= 0) {
                        gPipboyCurrentLine = gPipboyLinesCount;
                    }

                    // Back
                    const char* text1 = getmsg(&gPipboyMessageList, &gPipboyMessageListItem, 201);
                    pipboyDrawText(text1, PIPBOY_TEXT_ALIGNMENT_LEFT_COLUMN_CENTER, colorTable[992]);

                    if (gPipboyLinesCount >= 0) {
                        gPipboyCurrentLine = gPipboyLinesCount;
                    }

                    // Done
                    const char* text2 = getmsg(&gPipboyMessageList, &gPipboyMessageListItem, 214);
                    pipboyDrawText(text2, PIPBOY_TEXT_ALIGNMENT_RIGHT_COLUMN_CENTER, colorTable[992]);

                    win_draw_rect(gPipboyWindow, &gPipboyWindowContentRect);
                    coreDelayProcessingEvents(200);
                    pipboyWindowHandleStatus(1024);
                }
            } else {
                gsound_play_sfx_file("ib1p1xx1");
                blitBufferToBuffer(gPipboyFrmData[PIPBOY_FRM_BACKGROUND] + PIPBOY_WINDOW_WIDTH * 436 + 254, 350, 20, PIPBOY_WINDOW_WIDTH, gPipboyWindowBuffer + PIPBOY_WINDOW_WIDTH * 436 + 254, PIPBOY_WINDOW_WIDTH);

                if (gPipboyLinesCount >= 0) {
                    gPipboyCurrentLine = gPipboyLinesCount;
                }

                // Back
                const char* text1 = getmsg(&gPipboyMessageList, &gPipboyMessageListItem, 201);
                pipboyDrawText(text1, PIPBOY_TEXT_ALIGNMENT_LEFT_COLUMN_CENTER, colorTable[992]);

                if (gPipboyLinesCount >= 0) {
                    gPipboyCurrentLine = gPipboyLinesCount;
                }

                // More
                const char* text2 = getmsg(&gPipboyMessageList, &gPipboyMessageListItem, 200);
                pipboyDrawText(text2, PIPBOY_TEXT_ALIGNMENT_RIGHT_COLUMN_CENTER, colorTable[992]);

                win_draw_rect(gPipboyWindow, &gPipboyWindowContentRect);
                coreDelayProcessingEvents(200);

                _view_page += 1;

                pipboyRenderHolodiskText();
            }
            return;
        }

        if (a1 == 1027) {
            gsound_play_sfx_file("ib1p1xx1");
            blitBufferToBuffer(gPipboyFrmData[PIPBOY_FRM_BACKGROUND] + PIPBOY_WINDOW_WIDTH * 436 + 254, 350, 20, PIPBOY_WINDOW_WIDTH, gPipboyWindowBuffer + PIPBOY_WINDOW_WIDTH * 436 + 254, PIPBOY_WINDOW_WIDTH);

            if (gPipboyLinesCount >= 0) {
                gPipboyCurrentLine = gPipboyLinesCount;
            }

            // Back
            const char* text1 = getmsg(&gPipboyMessageList, &gPipboyMessageListItem, 201);
            pipboyDrawText(text1, PIPBOY_TEXT_ALIGNMENT_LEFT_COLUMN_CENTER, colorTable[992]);

            if (gPipboyLinesCount >= 0) {
                gPipboyCurrentLine = gPipboyLinesCount;
            }

            // More
            const char* text2 = getmsg(&gPipboyMessageList, &gPipboyMessageListItem, 200);
            pipboyDrawText(text2, PIPBOY_TEXT_ALIGNMENT_RIGHT_COLUMN_CENTER, colorTable[992]);

            win_draw_rect(gPipboyWindow, &gPipboyWindowContentRect);
            coreDelayProcessingEvents(200);

            _view_page -= 1;

            if (_view_page < 0) {
                pipboyWindowHandleStatus(1024);
                return;
            }
        } else {
            if (gPipboyMouseX > 395) {
                return;
            }

            gsound_play_sfx_file("ib1p1xx1");
            blitBufferToBuffer(gPipboyFrmData[PIPBOY_FRM_BACKGROUND] + PIPBOY_WINDOW_WIDTH * 436 + 254, 350, 20, PIPBOY_WINDOW_WIDTH, gPipboyWindowBuffer + PIPBOY_WINDOW_WIDTH * 436 + 254, PIPBOY_WINDOW_WIDTH);

            if (gPipboyLinesCount >= 0) {
                gPipboyCurrentLine = gPipboyLinesCount;
            }

            // Back
            const char* text1 = getmsg(&gPipboyMessageList, &gPipboyMessageListItem, 201);
            pipboyDrawText(text1, PIPBOY_TEXT_ALIGNMENT_LEFT_COLUMN_CENTER, colorTable[992]);

            if (gPipboyLinesCount >= 0) {
                gPipboyCurrentLine = gPipboyLinesCount;
            }

            // More
            const char* text2 = getmsg(&gPipboyMessageList, &gPipboyMessageListItem, 200);
            pipboyDrawText(text2, PIPBOY_TEXT_ALIGNMENT_RIGHT_COLUMN_CENTER, colorTable[992]);

            win_draw_rect(gPipboyWindow, &gPipboyWindowContentRect);
            coreDelayProcessingEvents(200);

            if (_view_page <= 0) {
                pipboyWindowHandleStatus(1024);
                return;
            }

            _view_page -= 1;
        }

        pipboyRenderHolodiskText();
        return;
    }

    if (a1 == 1025) {
        gsound_play_sfx_file("ib1p1xx1");
        pipboyDrawBackButton(colorTable[32747]);
        win_draw_rect(gPipboyWindow, &gPipboyWindowContentRect);
        coreDelayProcessingEvents(200);
        pipboyWindowHandleStatus(1024);
    }

    if (a1 <= gPipboyQuestLocationsCount) {
        gsound_play_sfx_file("ib1p1xx1");

        int v13 = 0;
        int index = 0;
        for (; index < gQuestsCount; index++) {
            QuestDescription* questDescription = &(gQuestDescriptions[index]);
            if (questDescription->displayThreshold <= game_global_vars[questDescription->gvar]) {
                if (v13 == a1 - 1) {
                    break;
                }

                v13 += 1;

                // Skip quests in the same location.
                //
                // FIXME: This code should be identical to the one in the
                // `pipboyWindowRenderQuestLocationList`. See buffer overread
                // bug involved.
                for (; index < gQuestsCount; index++) {
                    if (gQuestDescriptions[index].location != gQuestDescriptions[index + 1].location) {
                        break;
                    }
                }
            }
        }

        pipboyWindowDestroyButtons();
        blitBufferToBuffer(gPipboyFrmData[PIPBOY_FRM_BACKGROUND] + PIPBOY_WINDOW_WIDTH * PIPBOY_WINDOW_CONTENT_VIEW_Y + PIPBOY_WINDOW_CONTENT_VIEW_X,
            PIPBOY_WINDOW_CONTENT_VIEW_WIDTH,
            PIPBOY_WINDOW_CONTENT_VIEW_HEIGHT,
            PIPBOY_WINDOW_WIDTH,
            gPipboyWindowBuffer + PIPBOY_WINDOW_WIDTH * PIPBOY_WINDOW_CONTENT_VIEW_Y + PIPBOY_WINDOW_CONTENT_VIEW_X,
            PIPBOY_WINDOW_WIDTH);
        if (gPipboyLinesCount >= 0) {
            gPipboyCurrentLine = 0;
        }

        if (gPipboyLinesCount >= 1) {
            gPipboyCurrentLine = 1;
        }

        pipboyWindowCreateButtons(0, 0, true);

        QuestDescription* questDescription = &(gQuestDescriptions[index]);

        const char* text1 = getmsg(&gPipboyMessageList, &gPipboyMessageListItem, 210);
        const char* text2 = getmsg(&map_msg_file, &gPipboyMessageListItem, questDescription->location);
        char formattedText[1024];
        sprintf(formattedText, "%s %s", text2, text1);
        pipboyDrawText(formattedText, PIPBOY_TEXT_STYLE_UNDERLINE, colorTable[992]);

        if (gPipboyLinesCount >= 3) {
            gPipboyCurrentLine = 3;
        }

        int number = 1;
        for (; index < gQuestsCount; index++) {
            QuestDescription* questDescription = &(gQuestDescriptions[index]);
            if (game_global_vars[questDescription->gvar] >= questDescription->displayThreshold) {
                const char* text = getmsg(&gQuestsMessageList, &gPipboyMessageListItem, questDescription->description);
                char formattedText[1024];
                sprintf(formattedText, "%d. %s", number, text);
                number += 1;

                short beginnings[WORD_WRAP_MAX_COUNT];
                short count;
                if (wordWrap(formattedText, 350, beginnings, &count) == 0) {
                    for (int line = 0; line < count - 1; line += 1) {
                        char* beginning = formattedText + beginnings[line];
                        char* ending = formattedText + beginnings[line + 1];
                        char c = *ending;
                        *ending = '\0';

                        int flags;
                        int color;
                        if (game_global_vars[questDescription->gvar] < questDescription->completedThreshold) {
                            flags = 0;
                            color = colorTable[992];
                        } else {
                            flags = PIPBOY_TEXT_STYLE_STRIKE_THROUGH;
                            color = colorTable[8804];
                        }

                        pipboyDrawText(beginning, flags, color);

                        *ending = c;
                        gPipboyCurrentLine += 1;
                    }
                } else {
                    debugPrint("\n ** Word wrap error in pipboy! **\n");
                }
            }

            if (index != gQuestsCount - 1) {
                QuestDescription* nextQuestDescription = &(gQuestDescriptions[index + 1]);
                if (questDescription->location != nextQuestDescription->location) {
                    break;
                }
            }
        }

        pipboyDrawBackButton(colorTable[992]);
        win_draw_rect(gPipboyWindow, &gPipboyWindowContentRect);
        _stat_flag = 1;
    }
}

// [a1] is likely selected location, or -1 if nothing is selected
//
// 0x498734
void pipboyWindowRenderQuestLocationList(int a1)
{
    if (gPipboyLinesCount >= 0) {
        gPipboyCurrentLine = 0;
    }

    int flags = gPipboyWindowHolodisksCount != 0 ? PIPBOY_TEXT_ALIGNMENT_LEFT_COLUMN_CENTER : PIPBOY_TEXT_ALIGNMENT_CENTER;
    flags |= PIPBOY_TEXT_STYLE_UNDERLINE;

    // STATUS
    const char* statusText = getmsg(&gPipboyMessageList, &gPipboyMessageListItem, 202);
    pipboyDrawText(statusText, flags, colorTable[992]);

    if (gPipboyLinesCount >= 2) {
        gPipboyCurrentLine = 2;
    }

    gPipboyQuestLocationsCount = 0;

    for (int index = 0; index < gQuestsCount; index += 1) {
        QuestDescription* quest = &(gQuestDescriptions[index]);
        if (quest->displayThreshold > game_global_vars[quest->gvar]) {
            continue;
        }

        int color = (gPipboyCurrentLine - 1) / 2 == (a1 - 1) ? colorTable[32747] : colorTable[992];

        // Render location.
        const char* questLocation = getmsg(&map_msg_file, &gPipboyMessageListItem, quest->location);
        pipboyDrawText(questLocation, 0, color);

        gPipboyCurrentLine += 1;
        gPipboyQuestLocationsCount += 1;

        // Skip quests in the same location.
        //
        // FIXME: There is a buffer overread bug at the end of the loop. It does
        // not manifest because dynamically allocated memory blocks have special
        // footer guard. Location field is the first in the struct and matches
        // size of the guard. So on the final iteration it compares location of
        // the last quest with this special guard (0xBEEFCAFE).
        for (; index < gQuestsCount; index++) {
            if (gQuestDescriptions[index].location != gQuestDescriptions[index + 1].location) {
                break;
            }
        }
    }
}

// 0x4988A0
void pipboyRenderHolodiskText()
{
    blitBufferToBuffer(gPipboyFrmData[PIPBOY_FRM_BACKGROUND] + PIPBOY_WINDOW_WIDTH * PIPBOY_WINDOW_CONTENT_VIEW_Y + PIPBOY_WINDOW_CONTENT_VIEW_X,
        PIPBOY_WINDOW_CONTENT_VIEW_WIDTH,
        PIPBOY_WINDOW_CONTENT_VIEW_HEIGHT,
        PIPBOY_WINDOW_WIDTH,
        gPipboyWindowBuffer + PIPBOY_WINDOW_WIDTH * PIPBOY_WINDOW_CONTENT_VIEW_Y + PIPBOY_WINDOW_CONTENT_VIEW_X,
        PIPBOY_WINDOW_WIDTH);

    if (gPipboyLinesCount >= 0) {
        gPipboyCurrentLine = 0;
    }

    HolodiskDescription* holodisk = &(gHolodiskDescriptions[_holodisk]);

    int holodiskTextId;
    int linesCount = 0;

    gPipboyHolodiskLastPage = 0;

    for (holodiskTextId = holodisk->description; holodiskTextId < holodisk->description + 500; holodiskTextId += 1) {
        const char* text = getmsg(&gPipboyMessageList, &gPipboyMessageListItem, holodiskTextId);
        if (strcmp(text, "**END-DISK**") == 0) {
            break;
        }

        linesCount += 1;
        if (linesCount >= PIPBOY_HOLODISK_LINES_MAX) {
            linesCount = 0;
            gPipboyHolodiskLastPage += 1;
        }
    }

    if (holodiskTextId >= holodisk->description + 500) {
        debugPrint("\nPIPBOY: #1 Holodisk text end not found!\n");
    }

    holodiskTextId = holodisk->description;

    if (_view_page != 0) {
        int page = 0;
        int numberOfLines = 0;
        for (; holodiskTextId < holodiskTextId + 500; holodiskTextId += 1) {
            const char* line = getmsg(&gPipboyMessageList, &gPipboyMessageListItem, holodiskTextId);
            if (strcmp(line, "**END-DISK**") == 0) {
                debugPrint("\nPIPBOY: Premature page end in holodisk page search!\n");
                break;
            }

            numberOfLines += 1;
            if (numberOfLines >= PIPBOY_HOLODISK_LINES_MAX) {
                page += 1;
                if (page >= _view_page) {
                    break;
                }

                numberOfLines = 0;
            }
        }

        holodiskTextId += 1;

        if (holodiskTextId >= holodisk->description + 500) {
            debugPrint("\nPIPBOY: #2 Holodisk text end not found!\n");
        }
    } else {
        const char* name = getmsg(&gPipboyMessageList, &gPipboyMessageListItem, holodisk->name);
        pipboyDrawText(name, PIPBOY_TEXT_ALIGNMENT_CENTER | PIPBOY_TEXT_STYLE_UNDERLINE, colorTable[992]);
    }

    if (gPipboyHolodiskLastPage != 0) {
        // of
        const char* of = getmsg(&gPipboyMessageList, &gPipboyMessageListItem, 212);
        char formattedText[60]; // TODO: Size is probably wrong.
        sprintf(formattedText, "%d %s %d", _view_page + 1, of, gPipboyHolodiskLastPage + 1);

        int len = fontGetStringWidth(of);
        fontDrawText(gPipboyWindowBuffer + PIPBOY_WINDOW_WIDTH * 47 + 616 + 604 - len, formattedText, 350, PIPBOY_WINDOW_WIDTH, colorTable[992]);
    }

    if (gPipboyLinesCount >= 3) {
        gPipboyCurrentLine = 3;
    }

    for (int line = 0; line < PIPBOY_HOLODISK_LINES_MAX; line += 1) {
        const char* text = getmsg(&gPipboyMessageList, &gPipboyMessageListItem, holodiskTextId);
        if (strcmp(text, "**END-DISK**") == 0) {
            break;
        }

        if (strcmp(text, "**END-PAR**") == 0) {
            gPipboyCurrentLine += 1;
        } else {
            pipboyDrawText(text, PIPBOY_TEXT_NO_INDENT, colorTable[992]);
        }

        holodiskTextId += 1;
    }

    int moreOrDoneTextId;
    if (gPipboyHolodiskLastPage <= _view_page) {
        if (gPipboyLinesCount >= 0) {
            gPipboyCurrentLine = gPipboyLinesCount;
        }

        const char* back = getmsg(&gPipboyMessageList, &gPipboyMessageListItem, 201);
        pipboyDrawText(back, PIPBOY_TEXT_ALIGNMENT_LEFT_COLUMN_CENTER, colorTable[992]);

        if (gPipboyLinesCount >= 0) {
            gPipboyCurrentLine = gPipboyLinesCount;
        }

        moreOrDoneTextId = 214;
    } else {
        if (gPipboyLinesCount >= 0) {
            gPipboyCurrentLine = gPipboyLinesCount;
        }

        const char* back = getmsg(&gPipboyMessageList, &gPipboyMessageListItem, 201);
        pipboyDrawText(back, PIPBOY_TEXT_ALIGNMENT_LEFT_COLUMN_CENTER, colorTable[992]);

        if (gPipboyLinesCount >= 0) {
            gPipboyCurrentLine = gPipboyLinesCount;
        }

        moreOrDoneTextId = 200;
    }

    const char* moreOrDoneText = getmsg(&gPipboyMessageList, &gPipboyMessageListItem, moreOrDoneTextId);
    pipboyDrawText(moreOrDoneText, PIPBOY_TEXT_ALIGNMENT_RIGHT_COLUMN_CENTER, colorTable[992]);
    win_draw(gPipboyWindow);
}

// 0x498C40
int pipboyWindowRenderHolodiskList(int a1)
{
    if (gPipboyLinesCount >= 2) {
        gPipboyCurrentLine = 2;
    }

    int knownHolodisksCount = 0;
    for (int index = 0; index < gHolodisksCount; index++) {
        HolodiskDescription* holodisk = &(gHolodiskDescriptions[index]);
        if (game_global_vars[holodisk->gvar] != 0) {
            int color;
            if ((gPipboyCurrentLine - 2) / 2 == a1) {
                color = colorTable[32747];
            } else {
                color = colorTable[992];
            }

            const char* text = getmsg(&gPipboyMessageList, &gPipboyMessageListItem, holodisk->name);
            pipboyDrawText(text, PIPBOY_TEXT_ALIGNMENT_RIGHT_COLUMN, color);

            gPipboyCurrentLine++;
            knownHolodisksCount++;
        }
    }

    if (knownHolodisksCount != 0) {
        if (gPipboyLinesCount >= 0) {
            gPipboyCurrentLine = 0;
        }

        const char* text = getmsg(&gPipboyMessageList, &gPipboyMessageListItem, 211); // DATA
        pipboyDrawText(text, PIPBOY_TEXT_ALIGNMENT_RIGHT_COLUMN_CENTER | PIPBOY_TEXT_STYLE_UNDERLINE, colorTable[992]);
    }

    return knownHolodisksCount;
}

// 0x498D34
int _qscmp(const void* a1, const void* a2)
{
    STRUCT_664350* v1 = (STRUCT_664350*)a1;
    STRUCT_664350* v2 = (STRUCT_664350*)a2;

    return strcmp(v1->name, v2->name);
}

// 0x498D40
void pipboyWindowHandleAutomaps(int a1)
{
    if (a1 == 1024) {
        pipboyWindowDestroyButtons();
        blitBufferToBuffer(gPipboyFrmData[PIPBOY_FRM_BACKGROUND] + PIPBOY_WINDOW_WIDTH * PIPBOY_WINDOW_CONTENT_VIEW_Y + PIPBOY_WINDOW_CONTENT_VIEW_X,
            PIPBOY_WINDOW_CONTENT_VIEW_WIDTH,
            PIPBOY_WINDOW_CONTENT_VIEW_HEIGHT,
            PIPBOY_WINDOW_WIDTH,
            gPipboyWindowBuffer + PIPBOY_WINDOW_WIDTH * PIPBOY_WINDOW_CONTENT_VIEW_Y + PIPBOY_WINDOW_CONTENT_VIEW_X,
            PIPBOY_WINDOW_WIDTH);

        if (gPipboyLinesCount >= 0) {
            gPipboyCurrentLine = 0;
        }

        const char* title = getmsg(&gPipboyMessageList, &gPipboyMessageListItem, 205);
        pipboyDrawText(title, PIPBOY_TEXT_ALIGNMENT_CENTER | PIPBOY_TEXT_STYLE_UNDERLINE, colorTable[992]);

        _actcnt = _PrintAMList(-1);

        pipboyWindowCreateButtons(2, _actcnt, 0);

        win_draw_rect(gPipboyWindow, &gPipboyWindowContentRect);
        _amlst_mode = 0;
        return;
    }

    if (_amlst_mode != 0) {
        if (a1 == 1025 || a1 <= -1) {
            pipboyWindowHandleAutomaps(1024);
            gsound_play_sfx_file("ib1p1xx1");
        }

        if (a1 >= 1 && a1 <= _actcnt + 3) {
            gsound_play_sfx_file("ib1p1xx1");
            _PrintAMelevList(a1);
            draw_top_down_map_pipboy(gPipboyWindow, _sortlist[a1 - 1].field_6, _sortlist[a1 - 1].field_4);
            win_draw_rect(gPipboyWindow, &gPipboyWindowContentRect);
        }

        return;
    }

    if (a1 > 0 && a1 <= _actcnt) {
        gsound_play_sfx_file("ib1p1xx1");
        pipboyWindowDestroyButtons();
        _PrintAMList(a1);
        win_draw_rect(gPipboyWindow, &gPipboyWindowContentRect);
        _amcty_indx = _sortlist[a1 - 1].field_4;
        _actcnt = _PrintAMelevList(1);
        pipboyWindowCreateButtons(0, _actcnt + 2, 1);
        draw_top_down_map_pipboy(gPipboyWindow, _sortlist[0].field_6, _sortlist[0].field_4);
        win_draw_rect(gPipboyWindow, &gPipboyWindowContentRect);
        _amlst_mode = 1;
    }
}

// 0x498F30
int _PrintAMelevList(int a1)
{
    AutomapHeader* automapHeader;
    if (ReadAMList(&automapHeader) == -1) {
        return -1;
    }

    int v4 = 0;
    for (int elevation = 0; elevation < ELEVATION_COUNT; elevation++) {
        if (automapHeader->offsets[_amcty_indx][elevation] > 0) {
            _sortlist[v4].name = map_get_elev_idx(_amcty_indx, elevation);
            _sortlist[v4].field_4 = elevation;
            _sortlist[v4].field_6 = _amcty_indx;
            v4++;
        }
    }

    int mapCount = wmMapMaxCount();
    for (int map = 0; map < mapCount; map++) {
        if (map == _amcty_indx) {
            continue;
        }

        if (get_map_idx_same(_amcty_indx, map) == -1) {
            continue;
        }

        for (int elevation = 0; elevation < ELEVATION_COUNT; elevation++) {
            if (automapHeader->offsets[map][elevation] > 0) {
                _sortlist[v4].name = map_get_elev_idx(map, elevation);
                _sortlist[v4].field_4 = elevation;
                _sortlist[v4].field_6 = map;
                v4++;
            }
        }
    }

    blitBufferToBuffer(gPipboyFrmData[PIPBOY_FRM_BACKGROUND] + PIPBOY_WINDOW_WIDTH * PIPBOY_WINDOW_CONTENT_VIEW_Y + PIPBOY_WINDOW_CONTENT_VIEW_X,
        PIPBOY_WINDOW_CONTENT_VIEW_WIDTH,
        PIPBOY_WINDOW_CONTENT_VIEW_HEIGHT,
        PIPBOY_WINDOW_WIDTH,
        gPipboyWindowBuffer + PIPBOY_WINDOW_WIDTH * PIPBOY_WINDOW_CONTENT_VIEW_Y + PIPBOY_WINDOW_CONTENT_VIEW_X,
        PIPBOY_WINDOW_WIDTH);

    if (gPipboyLinesCount >= 0) {
        gPipboyCurrentLine = 0;
    }

    const char* msg = getmsg(&gPipboyMessageList, &gPipboyMessageListItem, 205);
    pipboyDrawText(msg, PIPBOY_TEXT_ALIGNMENT_CENTER | PIPBOY_TEXT_STYLE_UNDERLINE, colorTable[992]);

    if (gPipboyLinesCount >= 2) {
        gPipboyCurrentLine = 2;
    }

    const char* name = map_get_description_idx(_amcty_indx);
    pipboyDrawText(name, PIPBOY_TEXT_ALIGNMENT_CENTER, colorTable[992]);

    if (gPipboyLinesCount >= 4) {
        gPipboyCurrentLine = 4;
    }

    int selectedPipboyLine = (a1 - 1) * 2;

    for (int index = 0; index < v4; index++) {
        int color;
        if (gPipboyCurrentLine - 4 == selectedPipboyLine) {
            color = colorTable[32747];
        } else {
            color = colorTable[992];
        }

        pipboyDrawText(_sortlist[index].name, 0, color);
        gPipboyCurrentLine++;
    }

    pipboyDrawBackButton(colorTable[992]);

    return v4;
}

// 0x499150
int _PrintAMList(int a1)
{
    AutomapHeader* automapHeader;
    if (ReadAMList(&automapHeader) == -1) {
        return -1;
    }

    int count = 0;
    int index = 0;

    int mapCount = wmMapMaxCount();
    for (int map = 0; map < mapCount; map++) {
        int elevation;
        for (elevation = 0; elevation < ELEVATION_COUNT; elevation++) {
            if (automapHeader->offsets[map][elevation] > 0) {
                if (automapDisplayMap(map) == 0) {
                    break;
                }
            }
        }

        if (elevation < ELEVATION_COUNT) {
            int v7;
            if (count != 0) {
                v7 = 0;
                for (int index = 0; index < count; index++) {
                    if (is_map_idx_same(map, _sortlist[index].field_4)) {
                        break;
                    }

                    v7++;
                }
            } else {
                v7 = 0;
            }

            if (v7 == count) {
                _sortlist[count].name = map_get_short_name(map);
                _sortlist[count].field_4 = map;
                count++;
            }
        }
    }

    if (count != 0) {
        if (count > 1) {
            qsort(_sortlist, count, sizeof(*_sortlist), _qscmp);
        }

        blitBufferToBuffer(gPipboyFrmData[PIPBOY_FRM_BACKGROUND] + PIPBOY_WINDOW_WIDTH * PIPBOY_WINDOW_CONTENT_VIEW_Y + PIPBOY_WINDOW_CONTENT_VIEW_X,
            PIPBOY_WINDOW_CONTENT_VIEW_WIDTH,
            PIPBOY_WINDOW_CONTENT_VIEW_HEIGHT,
            PIPBOY_WINDOW_WIDTH,
            gPipboyWindowBuffer + PIPBOY_WINDOW_WIDTH * PIPBOY_WINDOW_CONTENT_VIEW_Y + PIPBOY_WINDOW_CONTENT_VIEW_X,
            PIPBOY_WINDOW_WIDTH);

        if (gPipboyLinesCount >= 0) {
            gPipboyCurrentLine = 0;
        }

        const char* msg = getmsg(&gPipboyMessageList, &gPipboyMessageListItem, 205);
        pipboyDrawText(msg, PIPBOY_TEXT_ALIGNMENT_CENTER | PIPBOY_TEXT_STYLE_UNDERLINE, colorTable[992]);

        if (gPipboyLinesCount >= 2) {
            gPipboyCurrentLine = 2;
        }

        for (int index = 0; index < count; index++) {
            int color;
            if (gPipboyCurrentLine - 1 == a1) {
                color = colorTable[32747];
            } else {
                color = colorTable[992];
            }

            pipboyDrawText(_sortlist[index].name, 0, color);
            gPipboyCurrentLine++;
        }
    }

    return count;
}

// 0x49932C
void pipboyHandleVideoArchive(int a1)
{
    if (a1 == 1024) {
        pipboyWindowDestroyButtons();
        _view_page = pipboyRenderVideoArchive(-1);
        pipboyWindowCreateButtons(2, _view_page, false);
    } else if (a1 >= 0 && a1 <= _view_page) {
        gsound_play_sfx_file("ib1p1xx1");

        pipboyRenderVideoArchive(a1);

        int movie;
        for (movie = 2; movie < 16; movie++) {
            if (gmovie_has_been_played(movie)) {
                a1--;
                if (a1 <= 0) {
                    break;
                }
            }
        }

        if (movie <= MOVIE_COUNT) {
            gmovie_play(movie, GAME_MOVIE_FADE_IN | GAME_MOVIE_FADE_OUT | GAME_MOVIE_PAUSE_MUSIC);
        } else {
            debugPrint("\n ** Selected movie not found in list! **\n");
        }

        fontSetCurrent(101);

        gPipboyLastEventTimestamp = _get_time();
        pipboyRenderVideoArchive(-1);
    }
}

// 0x4993DC
int pipboyRenderVideoArchive(int a1)
{
    const char* text;
    int i;
    int v12;
    int msg_num;
    int v5;
    int v8;
    int v9;

    blitBufferToBuffer(gPipboyFrmData[PIPBOY_FRM_BACKGROUND] + PIPBOY_WINDOW_WIDTH * PIPBOY_WINDOW_CONTENT_VIEW_Y + PIPBOY_WINDOW_CONTENT_VIEW_X,
        PIPBOY_WINDOW_CONTENT_VIEW_WIDTH,
        PIPBOY_WINDOW_CONTENT_VIEW_HEIGHT,
        PIPBOY_WINDOW_WIDTH,
        gPipboyWindowBuffer + PIPBOY_WINDOW_WIDTH * PIPBOY_WINDOW_CONTENT_VIEW_Y + PIPBOY_WINDOW_CONTENT_VIEW_X,
        PIPBOY_WINDOW_WIDTH);

    if (gPipboyLinesCount >= 0) {
        gPipboyCurrentLine = 0;
    }

    // VIDEO ARCHIVES
    text = getmsg(&gPipboyMessageList, &gPipboyMessageListItem, 206);
    pipboyDrawText(text, PIPBOY_TEXT_ALIGNMENT_CENTER | PIPBOY_TEXT_STYLE_UNDERLINE, colorTable[992]);

    if (gPipboyLinesCount >= 2) {
        gPipboyCurrentLine = 2;
    }

    v5 = 0;
    v12 = a1 - 1;

    // 502 - Elder Speech
    // ...
    // 516 - Credits
    msg_num = 502;

    for (i = 2; i < 16; i++) {
        if (gmovie_has_been_played(i)) {
            v8 = v5++;
            if (v8 == v12) {
                v9 = colorTable[32747];
            } else {
                v9 = colorTable[992];
            }

            text = getmsg(&gPipboyMessageList, &gPipboyMessageListItem, msg_num);
            pipboyDrawText(text, 0, v9);

            gPipboyCurrentLine++;
        }

        msg_num++;
    }

    win_draw_rect(gPipboyWindow, &gPipboyWindowContentRect);

    return v5;
}

// 0x499518
void pipboyHandleAlarmClock(int a1)
{
    if (a1 == 1024) {
        if (critter_can_obj_dude_rest()) {
            pipboyWindowDestroyButtons();
            pipboyWindowRenderRestOptions(0);
            pipboyWindowCreateButtons(5, gPipboyRestOptionsCount, false);
        } else {
            gsound_play_sfx_file("iisxxxx1");

            // You cannot rest at this location!
            const char* text = getmsg(&gPipboyMessageList, &gPipboyMessageListItem, 215);
            dialog_out(text, NULL, 0, 192, 135, colorTable[32328], 0, colorTable[32328], DIALOG_BOX_LARGE);
        }
    } else if (a1 >= 4 && a1 <= 17) {
        gsound_play_sfx_file("ib1p1xx1");

        pipboyWindowRenderRestOptions(a1 - 3);

        int duration = a1 - 4;
        int minutes = 0;
        int hours = 0;
        int v10 = 0;

        switch (duration) {
        case PIPBOY_REST_DURATION_TEN_MINUTES:
            pipboyRest(0, 10, 0);
            break;
        case PIPBOY_REST_DURATION_THIRTY_MINUTES:
            pipboyRest(0, 30, 0);
            break;
        case PIPBOY_REST_DURATION_ONE_HOUR:
        case PIPBOY_REST_DURATION_TWO_HOURS:
        case PIPBOY_REST_DURATION_THREE_HOURS:
        case PIPBOY_REST_DURATION_FOUR_HOURS:
        case PIPBOY_REST_DURATION_FIVE_HOURS:
        case PIPBOY_REST_DURATION_SIX_HOURS:
            pipboyRest(duration - 1, 0, 0);
            break;
        case PIPBOY_REST_DURATION_UNTIL_MORNING:
            _ClacTime(&hours, &minutes, 8);
            pipboyRest(hours, minutes, 0);
            break;
        case PIPBOY_REST_DURATION_UNTIL_NOON:
            _ClacTime(&hours, &minutes, 12);
            pipboyRest(hours, minutes, 0);
            break;
        case PIPBOY_REST_DURATION_UNTIL_EVENING:
            _ClacTime(&hours, &minutes, 18);
            pipboyRest(hours, minutes, 0);
            break;
        case PIPBOY_REST_DURATION_UNTIL_MIDNIGHT:
            _ClacTime(&hours, &minutes, 0);
            if (pipboyRest(hours, minutes, 0) == 0) {
                pipboyDrawNumber(0, 4, PIPBOY_WINDOW_TIME_X, PIPBOY_WINDOW_TIME_Y);
            }
            win_draw(gPipboyWindow);
            break;
        case PIPBOY_REST_DURATION_UNTIL_HEALED:
        case PIPBOY_REST_DURATION_UNTIL_PARTY_HEALED:
            pipboyRest(0, 0, duration);
            break;
        }

        gsound_play_sfx_file("ib2lu1x1");

        pipboyWindowRenderRestOptions(0);
    }
}

// 0x4996B4
void pipboyWindowRenderRestOptions(int a1)
{
    const char* text;

    blitBufferToBuffer(gPipboyFrmData[PIPBOY_FRM_BACKGROUND] + PIPBOY_WINDOW_WIDTH * PIPBOY_WINDOW_CONTENT_VIEW_Y + PIPBOY_WINDOW_CONTENT_VIEW_X,
        PIPBOY_WINDOW_CONTENT_VIEW_WIDTH,
        PIPBOY_WINDOW_CONTENT_VIEW_HEIGHT,
        PIPBOY_WINDOW_WIDTH,
        gPipboyWindowBuffer + PIPBOY_WINDOW_WIDTH * PIPBOY_WINDOW_CONTENT_VIEW_Y + PIPBOY_WINDOW_CONTENT_VIEW_X,
        PIPBOY_WINDOW_WIDTH);

    if (gPipboyLinesCount >= 0) {
        gPipboyCurrentLine = 0;
    }

    // ALARM CLOCK
    text = getmsg(&gPipboyMessageList, &gPipboyMessageListItem, 300);
    pipboyDrawText(text, PIPBOY_TEXT_ALIGNMENT_CENTER | PIPBOY_TEXT_STYLE_UNDERLINE, colorTable[992]);

    if (gPipboyLinesCount >= 5) {
        gPipboyCurrentLine = 5;
    }

    pipboyDrawHitPoints();

    // NOTE: I don't know if this +1 was a result of compiler optimization or it
    // was written like this in the first place.
    for (int option = 1; option < gPipboyRestOptionsCount + 1; option++) {
        // 302 - Rest for ten minutes
        // ...
        // 315 - Rest until party is healed
        text = getmsg(&gPipboyMessageList, &gPipboyMessageListItem, 302 + option - 1);
        int color = option == a1 ? colorTable[32747] : colorTable[992];

        pipboyDrawText(text, 0, color);

        gPipboyCurrentLine++;
    }

    win_draw_rect(gPipboyWindow, &gPipboyWindowContentRect);
}

// 0x4997B8
void pipboyDrawHitPoints()
{
    int max_hp;
    int cur_hp;
    char* text;
    char msg[64];
    int len;

    blitBufferToBuffer(gPipboyFrmData[PIPBOY_FRM_BACKGROUND] + 66 * PIPBOY_WINDOW_WIDTH + 254,
        350,
        10,
        PIPBOY_WINDOW_WIDTH,
        gPipboyWindowBuffer + 66 * PIPBOY_WINDOW_WIDTH + 254,
        PIPBOY_WINDOW_WIDTH);

    max_hp = critterGetStat(obj_dude, STAT_MAXIMUM_HIT_POINTS);
    cur_hp = critter_get_hits(obj_dude);
    text = getmsg(&gPipboyMessageList, &gPipboyMessageListItem, 301); // Hit Points
    sprintf(msg, "%s %d/%d", text, cur_hp, max_hp);
    len = fontGetStringWidth(msg);
    fontDrawText(gPipboyWindowBuffer + 66 * PIPBOY_WINDOW_WIDTH + 254 + (350 - len) / 2, msg, PIPBOY_WINDOW_WIDTH, PIPBOY_WINDOW_WIDTH, colorTable[992]);
}

// 0x4998C0
void pipboyWindowCreateButtons(int start, int count, bool a3)
{
    fontSetCurrent(101);

    int height = fontGetLineHeight();

    gPipboyWindowButtonStart = start;
    gPipboyWindowButtonCount = count;

    if (count != 0) {
        int y = start * height + PIPBOY_WINDOW_CONTENT_VIEW_Y;
        int eventCode = start + 505;
        for (int index = start; index < 22; index++) {
            if (gPipboyWindowButtonCount + gPipboyWindowButtonStart <= index) {
                break;
            }

            _HotLines[index] = buttonCreate(gPipboyWindow, 254, y, 350, height, -1, -1, -1, eventCode, NULL, NULL, NULL, BUTTON_FLAG_TRANSPARENT);
            y += height * 2;
            eventCode += 1;
        }
    }

    if (a3) {
        _button = buttonCreate(gPipboyWindow, 254, height * gPipboyLinesCount + PIPBOY_WINDOW_CONTENT_VIEW_Y, 350, height, -1, -1, -1, 528, NULL, NULL, NULL, BUTTON_FLAG_TRANSPARENT);
    }
}

// 0x4999C0
void pipboyWindowDestroyButtons()
{
    if (gPipboyWindowButtonCount != 0) {
        // NOTE: There is a buffer overread bug. In original binary it leads to
        // reading continuation (from 0x6644B8 onwards), which finally destroys
        // button in `gPipboyWindow` (id #3), which corresponds to Skilldex
        // button. Other buttons can be destroyed depending on the last mouse
        // position. I was not able to replicate this exact behaviour with MSVC.
        // So here is a small fix, which is an exception to "Do not fix vanilla
        // bugs" strategy.
        int end = gPipboyWindowButtonStart + gPipboyWindowButtonCount;
        if (end > 22) {
            end = 22;
        }

        for (int index = gPipboyWindowButtonStart; index < end; index++) {
            buttonDestroy(_HotLines[index]);
        }
    }

    if (_hot_back_line) {
        buttonDestroy(_button);
    }

    gPipboyWindowButtonCount = 0;
    _hot_back_line = 0;
}

// 0x499A24
bool pipboyRest(int hours, int minutes, int duration)
{
    gmouse_set_cursor(MOUSE_CURSOR_WAIT_WATCH);

    bool rc = false;

    if (duration == 0) {
        int hoursInMinutes = hours * 60;
        double v1 = (double)hoursInMinutes + (double)minutes;
        double v2 = v1 * (1.0 / 1440.0) * 3.5 + 0.25;
        double v3 = (double)minutes / v1 * v2;
        if (minutes != 0) {
            int gameTime = gameTimeGetTime();

            double v4 = v3 * 20.0;
            int v5 = 0;
            for (int v5 = 0; v5 < (int)v4; v5++) {
                if (rc) {
                    break;
                }

                unsigned int start = _get_time();

                unsigned int v6 = (unsigned int)((double)v5 / v4 * ((double)minutes * 600.0) + (double)gameTime);
                unsigned int nextEventTime = queueGetNextEventTime();
                if (v6 >= nextEventTime) {
                    gameTimeSetTime(nextEventTime + 1);
                    if (queueProcessEvents()) {
                        rc = true;
                        debugPrint("PIPBOY: Returning from Queue trigger...\n");
                        _proc_bail_flag = 1;
                        break;
                    }

                    if (game_user_wants_to_quit != 0) {
                        rc = true;
                    }
                }

                if (!rc) {
                    gameTimeSetTime(v6);
                    if (_get_input() == KEY_ESCAPE || game_user_wants_to_quit != 0) {
                        rc = true;
                    }

                    pipboyDrawNumber(gameTimeGetHour(), 4, PIPBOY_WINDOW_TIME_X, PIPBOY_WINDOW_TIME_Y);
                    pipboyDrawDate();
                    win_draw(gPipboyWindow);

                    while (getTicksSince(start) < 50) {
                    }
                }
            }

            if (!rc) {
                gameTimeSetTime(gameTime + 600 * minutes);

                if (_Check4Health(minutes)) {
                    // NOTE: Uninline.
                    _AddHealth();
                }
            }

            pipboyDrawNumber(gameTimeGetHour(), 4, PIPBOY_WINDOW_TIME_X, PIPBOY_WINDOW_TIME_Y);
            pipboyDrawDate();
            pipboyDrawHitPoints();
            win_draw(gPipboyWindow);
        }

        if (hours != 0 && !rc) {
            int gameTime = gameTimeGetTime();
            double v7 = (v2 - v3) * 20.0;

            for (int hour = 0; hour < (int)v7; hour++) {
                if (rc) {
                    break;
                }

                unsigned int start = _get_time();

                if (_get_input() == KEY_ESCAPE || game_user_wants_to_quit != 0) {
                    rc = true;
                }

                unsigned int v8 = (unsigned int)((double)hour / v7 * (hours * GAME_TIME_TICKS_PER_HOUR) + gameTime);
                unsigned int nextEventTime = queueGetNextEventTime();
                if (!rc && v8 >= nextEventTime) {
                    gameTimeSetTime(nextEventTime + 1);

                    if (queueProcessEvents()) {
                        rc = true;
                        debugPrint("PIPBOY: Returning from Queue trigger...\n");
                        _proc_bail_flag = 1;
                        break;
                    }

                    if (game_user_wants_to_quit != 0) {
                        rc = true;
                    }
                }

                if (!rc) {
                    gameTimeSetTime(v8);

                    int healthToAdd = (int)((double)hoursInMinutes / v7);
                    if (_Check4Health(healthToAdd)) {
                        // NOTE: Uninline.
                        _AddHealth();
                    }

                    pipboyDrawNumber(gameTimeGetHour(), 4, PIPBOY_WINDOW_TIME_X, PIPBOY_WINDOW_TIME_Y);
                    pipboyDrawDate();
                    pipboyDrawHitPoints();
                    win_draw(gPipboyWindow);

                    while (getTicksSince(start) < 50) {
                    }
                }
            }

            if (!rc) {
                gameTimeSetTime(gameTime + GAME_TIME_TICKS_PER_HOUR * hours);
            }

            pipboyDrawNumber(gameTimeGetHour(), 4, PIPBOY_WINDOW_TIME_X, PIPBOY_WINDOW_TIME_Y);
            pipboyDrawDate();
            pipboyDrawHitPoints();
            win_draw(gPipboyWindow);
        }
    } else if (duration == PIPBOY_REST_DURATION_UNTIL_HEALED || duration == PIPBOY_REST_DURATION_UNTIL_PARTY_HEALED) {
        int currentHp = critter_get_hits(obj_dude);
        int maxHp = critterGetStat(obj_dude, STAT_MAXIMUM_HIT_POINTS);
        if (currentHp != maxHp
            || (duration == PIPBOY_REST_DURATION_UNTIL_PARTY_HEALED && partyMemberNeedsHealing())) {
            // First pass - healing dude is the top priority.
            int hpToHeal = maxHp - currentHp;
            int healingRate = critterGetStat(obj_dude, STAT_HEALING_RATE);
            int hoursToHeal = (int)((double)hpToHeal / (double)healingRate * 3.0);
            while (!rc && hoursToHeal != 0) {
                if (hoursToHeal <= 24) {
                    rc = pipboyRest(hoursToHeal, 0, 0);
                    hoursToHeal = 0;
                } else {
                    rc = pipboyRest(24, 0, 0);
                    hoursToHeal -= 24;
                }
            }

            // Second pass - attempt to heal delayed damage to dude (via poison
            // or radiation), and remaining party members. This process is
            // performed in 3 hour increments.
            currentHp = critter_get_hits(obj_dude);
            maxHp = critterGetStat(obj_dude, STAT_MAXIMUM_HIT_POINTS);
            hpToHeal = maxHp - currentHp;

            if (duration == PIPBOY_REST_DURATION_UNTIL_PARTY_HEALED) {
                int partyHpToHeal = partyMemberMaxHealingNeeded();
                if (partyHpToHeal > hpToHeal) {
                    hpToHeal = partyHpToHeal;
                }
            }

            while (!rc && hpToHeal != 0) {
                currentHp = critter_get_hits(obj_dude);
                maxHp = critterGetStat(obj_dude, STAT_MAXIMUM_HIT_POINTS);
                hpToHeal = maxHp - currentHp;

                if (duration == PIPBOY_REST_DURATION_UNTIL_PARTY_HEALED) {
                    int partyHpToHeal = partyMemberMaxHealingNeeded();
                    if (partyHpToHeal > hpToHeal) {
                        hpToHeal = partyHpToHeal;
                    }
                }

                rc = pipboyRest(3, 0, 0);
            }
        } else {
            // No one needs healing.
            gmouse_set_cursor(MOUSE_CURSOR_ARROW);
            return rc;
        }
    }

    int gameTime = gameTimeGetTime();
    int nextEventGameTime = queueGetNextEventTime();
    if (gameTime > nextEventGameTime) {
        if (queueProcessEvents()) {
            debugPrint("PIPBOY: Returning from Queue trigger...\n");
            _proc_bail_flag = 1;
            rc = true;
        }
    }

    pipboyDrawNumber(gameTimeGetHour(), 4, PIPBOY_WINDOW_TIME_X, PIPBOY_WINDOW_TIME_Y);
    pipboyDrawDate();
    win_draw(gPipboyWindow);

    gmouse_set_cursor(MOUSE_CURSOR_ARROW);

    return rc;
}

// 0x499FCC
bool _Check4Health(int a1)
{
    _rest_time += a1;

    if (_rest_time < 180) {
        return false;
    }

    debugPrint("\n health added!\n");
    _rest_time = 0;

    return true;
}

// NOTE: Inlined.
bool _AddHealth()
{
    partyMemberRestingHeal(3);

    int currentHp = critter_get_hits(obj_dude);
    int maxHp = critterGetStat(obj_dude, STAT_MAXIMUM_HIT_POINTS);
    return currentHp == maxHp;
}

// Returns [hours] and [minutes] needed to rest until [wakeUpHour].
void _ClacTime(int* hours, int* minutes, int wakeUpHour)
{
    int gameTimeHour = gameTimeGetHour();

    *hours = gameTimeHour / 100;
    *minutes = gameTimeHour % 100;

    if (*hours != wakeUpHour || *minutes != 0) {
        *hours = wakeUpHour - *hours;
        if (*hours < 0) {
            *hours += 24;
            if (*minutes != 0) {
                *hours -= 1;
                *minutes = 60 - *minutes;
            }
        } else {
            if (*minutes != 0) {
                *hours -= 1;
                *minutes = 60 - *minutes;
                if (*hours < 0) {
                    *hours = 23;
                }
            }
        }
    } else {
        *hours = 24;
    }
}

// 0x49A0C8
int pipboyRenderScreensaver()
{
    PipboyBomb bombs[PIPBOY_BOMB_COUNT];

    mouse_get_position(&gPipboyPreviousMouseX, &gPipboyPreviousMouseY);

    for (int index = 0; index < PIPBOY_BOMB_COUNT; index += 1) {
        bombs[index].field_10 = 0;
    }

    gmouse_disable(0);

    unsigned char* buf = (unsigned char*)internal_malloc(412 * 374);
    if (buf == NULL) {
        return -1;
    }

    blitBufferToBuffer(gPipboyWindowBuffer + PIPBOY_WINDOW_WIDTH * PIPBOY_WINDOW_CONTENT_VIEW_Y + PIPBOY_WINDOW_CONTENT_VIEW_X,
        PIPBOY_WINDOW_CONTENT_VIEW_WIDTH,
        PIPBOY_WINDOW_CONTENT_VIEW_HEIGHT,
        PIPBOY_WINDOW_WIDTH,
        buf,
        PIPBOY_WINDOW_CONTENT_VIEW_WIDTH);

    blitBufferToBuffer(gPipboyFrmData[PIPBOY_FRM_BACKGROUND] + PIPBOY_WINDOW_WIDTH * PIPBOY_WINDOW_CONTENT_VIEW_Y + PIPBOY_WINDOW_CONTENT_VIEW_X,
        PIPBOY_WINDOW_CONTENT_VIEW_WIDTH,
        PIPBOY_WINDOW_CONTENT_VIEW_HEIGHT,
        PIPBOY_WINDOW_WIDTH,
        gPipboyWindowBuffer + PIPBOY_WINDOW_WIDTH * PIPBOY_WINDOW_CONTENT_VIEW_Y + PIPBOY_WINDOW_CONTENT_VIEW_X,
        PIPBOY_WINDOW_WIDTH);

    int v31 = 50;
    while (true) {
        unsigned int time = _get_time();

        mouse_get_position(&gPipboyMouseX, &gPipboyMouseY);
        if (_get_input() != -1 || gPipboyPreviousMouseX != gPipboyMouseX || gPipboyPreviousMouseY != gPipboyMouseY) {
            break;
        }

        double random = randomBetween(0, PIPBOY_RAND_MAX);

        // TODO: Figure out what this constant means. Probably somehow related
        // to PIPBOY_RAND_MAX.
        if (random < 3047.3311) {
            int index = 0;
            for (; index < PIPBOY_BOMB_COUNT; index += 1) {
                if (bombs[index].field_10 == 0) {
                    break;
                }
            }

            if (index < PIPBOY_BOMB_COUNT) {
                PipboyBomb* bomb = &(bombs[index]);
                int v27 = (350 - gPipboyFrmSizes[PIPBOY_FRM_BOMB].width / 4) + (406 - gPipboyFrmSizes[PIPBOY_FRM_BOMB].height / 4);
                int v5 = (int)((double)randomBetween(0, PIPBOY_RAND_MAX) / (double)PIPBOY_RAND_MAX * (double)v27);
                int v6 = gPipboyFrmSizes[PIPBOY_FRM_BOMB].height / 4;
                if (PIPBOY_WINDOW_CONTENT_VIEW_HEIGHT - v6 >= v5) {
                    bomb->x = 602;
                    bomb->y = v5 + 48;
                } else {
                    bomb->x = v5 - (PIPBOY_WINDOW_CONTENT_VIEW_HEIGHT - v6) + PIPBOY_WINDOW_CONTENT_VIEW_X + gPipboyFrmSizes[PIPBOY_FRM_BOMB].width / 4;
                    bomb->y = PIPBOY_WINDOW_CONTENT_VIEW_Y - gPipboyFrmSizes[PIPBOY_FRM_BOMB].height + 2;
                }

                bomb->field_10 = 1;
                bomb->field_8 = (float)((double)randomBetween(0, PIPBOY_RAND_MAX) * (2.75 / PIPBOY_RAND_MAX) + 0.15);
                bomb->field_C = 0;
            }
        }

        if (v31 == 0) {
            blitBufferToBuffer(gPipboyFrmData[PIPBOY_FRM_BACKGROUND] + PIPBOY_WINDOW_WIDTH * PIPBOY_WINDOW_CONTENT_VIEW_Y + PIPBOY_WINDOW_CONTENT_VIEW_X,
                PIPBOY_WINDOW_CONTENT_VIEW_WIDTH,
                PIPBOY_WINDOW_CONTENT_VIEW_HEIGHT,
                PIPBOY_WINDOW_WIDTH,
                gPipboyWindowBuffer + PIPBOY_WINDOW_WIDTH * PIPBOY_WINDOW_CONTENT_VIEW_Y + PIPBOY_WINDOW_CONTENT_VIEW_X,
                PIPBOY_WINDOW_WIDTH);
        }

        for (int index = 0; index < PIPBOY_BOMB_COUNT; index++) {
            PipboyBomb* bomb = &(bombs[index]);
            if (bomb->field_10 != 1) {
                continue;
            }

            int srcWidth = gPipboyFrmSizes[PIPBOY_FRM_BOMB].width;
            int srcHeight = gPipboyFrmSizes[PIPBOY_FRM_BOMB].height;
            int destX = bomb->x;
            int destY = bomb->y;
            int srcY = 0;
            int srcX = 0;

            if (destX >= PIPBOY_WINDOW_CONTENT_VIEW_X) {
                if (destX + gPipboyFrmSizes[PIPBOY_FRM_BOMB].width >= 604) {
                    srcWidth = 604 - destX;
                    if (srcWidth < 1) {
                        bomb->field_10 = 0;
                    }
                }
            } else {
                srcX = PIPBOY_WINDOW_CONTENT_VIEW_X - destX;
                if (srcX >= gPipboyFrmSizes[PIPBOY_FRM_BOMB].width) {
                    bomb->field_10 = 0;
                }
                destX = PIPBOY_WINDOW_CONTENT_VIEW_X;
                srcWidth = gPipboyFrmSizes[PIPBOY_FRM_BOMB].width - srcX;
            }

            if (destY >= PIPBOY_WINDOW_CONTENT_VIEW_Y) {
                if (destY + gPipboyFrmSizes[PIPBOY_FRM_BOMB].height >= 452) {
                    srcHeight = 452 - destY;
                    if (srcHeight < 1) {
                        bomb->field_10 = 0;
                    }
                }
            } else {
                if (destY + gPipboyFrmSizes[PIPBOY_FRM_BOMB].height < PIPBOY_WINDOW_CONTENT_VIEW_Y) {
                    bomb->field_10 = 0;
                }

                srcY = PIPBOY_WINDOW_CONTENT_VIEW_Y - destY;
                srcHeight = gPipboyFrmSizes[PIPBOY_FRM_BOMB].height - srcY;
                destY = PIPBOY_WINDOW_CONTENT_VIEW_Y;
            }

            if (bomb->field_10 == 1 && v31 == 0) {
                blitBufferToBufferTrans(
                    gPipboyFrmData[PIPBOY_FRM_BOMB] + gPipboyFrmSizes[PIPBOY_FRM_BOMB].width * srcY + srcX,
                    srcWidth,
                    srcHeight,
                    gPipboyFrmSizes[PIPBOY_FRM_BOMB].width,
                    gPipboyWindowBuffer + PIPBOY_WINDOW_WIDTH * destY + destX,
                    PIPBOY_WINDOW_WIDTH);
            }

            bomb->field_C += bomb->field_8;
            if (bomb->field_C >= 1.0) {
                bomb->x = (int)((float)bomb->x - bomb->field_C);
                bomb->y = (int)((float)bomb->y + bomb->field_C);
                bomb->field_C = 0.0;
            }
        }

        if (v31 != 0) {
            v31 -= 1;
        } else {
            win_draw_rect(gPipboyWindow, &gPipboyWindowContentRect);
            while (getTicksSince(time) < 50) {
            }
        }
    }

    blitBufferToBuffer(buf,
        PIPBOY_WINDOW_CONTENT_VIEW_WIDTH,
        PIPBOY_WINDOW_CONTENT_VIEW_HEIGHT,
        PIPBOY_WINDOW_CONTENT_VIEW_WIDTH,
        gPipboyWindowBuffer + PIPBOY_WINDOW_WIDTH * PIPBOY_WINDOW_CONTENT_VIEW_Y + PIPBOY_WINDOW_CONTENT_VIEW_X,
        PIPBOY_WINDOW_WIDTH);

    internal_free(buf);

    win_draw_rect(gPipboyWindow, &gPipboyWindowContentRect);
    gmouse_enable();

    return 0;
}

// 0x49A5D4
int questInit()
{
    if (gQuestDescriptions != NULL) {
        internal_free(gQuestDescriptions);
        gQuestDescriptions = NULL;
    }

    gQuestsCount = 0;

    message_exit(&gQuestsMessageList);

    if (!message_init(&gQuestsMessageList)) {
        return -1;
    }

    if (!message_load(&gQuestsMessageList, "game\\quests.msg")) {
        return -1;
    }

    File* stream = fileOpen("data\\quests.txt", "rt");
    if (stream == NULL) {
        return -1;
    }

    char string[256];
    while (fileReadString(string, 256, stream)) {
        const char* delim = " \t,";
        char* tok;
        QuestDescription entry;

        char* pch = string;
        while (isspace(*pch)) {
            pch++;
        }

        if (*pch == '#') {
            continue;
        }

        tok = strtok(pch, delim);
        if (tok == NULL) {
            continue;
        }

        entry.location = atoi(tok);

        tok = strtok(NULL, delim);
        if (tok == NULL) {
            continue;
        }

        entry.description = atoi(tok);

        tok = strtok(NULL, delim);
        if (tok == NULL) {
            continue;
        }

        entry.gvar = atoi(tok);

        tok = strtok(NULL, delim);
        if (tok == NULL) {
            continue;
        }

        entry.displayThreshold = atoi(tok);

        tok = strtok(NULL, delim);
        if (tok == NULL) {
            continue;
        }

        entry.completedThreshold = atoi(tok);

        QuestDescription* entries = (QuestDescription*)internal_realloc(gQuestDescriptions, sizeof(*gQuestDescriptions) * (gQuestsCount + 1));
        if (entries == NULL) {
            goto err;
        }

        memcpy(&(entries[gQuestsCount]), &entry, sizeof(entry));

        gQuestDescriptions = entries;
        gQuestsCount++;
    }

    qsort(gQuestDescriptions, gQuestsCount, sizeof(*gQuestDescriptions), questDescriptionCompare);

    fileClose(stream);

    return 0;

err:

    fileClose(stream);

    return -1;
}

// 0x49A7E4
void questFree()
{
    if (gQuestDescriptions != NULL) {
        internal_free(gQuestDescriptions);
        gQuestDescriptions = NULL;
    }

    gQuestsCount = 0;

    message_exit(&gQuestsMessageList);
}

// 0x49A818
int questDescriptionCompare(const void* a1, const void* a2)
{
    QuestDescription* v1 = (QuestDescription*)a1;
    QuestDescription* v2 = (QuestDescription*)a2;
    return v1->location - v2->location;
}

// 0x49A824
int holodiskInit()
{
    if (gHolodiskDescriptions != NULL) {
        internal_free(gHolodiskDescriptions);
        gHolodiskDescriptions = NULL;
    }

    gHolodisksCount = 0;

    File* stream = fileOpen("data\\holodisk.txt", "rt");
    if (stream == NULL) {
        return -1;
    }

    char str[256];
    while (fileReadString(str, sizeof(str), stream)) {
        const char* delim = " \t,";
        char* tok;
        HolodiskDescription entry;

        char* ch = str;
        while (isspace(*ch)) {
            ch++;
        }

        if (*ch == '#') {
            continue;
        }

        tok = strtok(ch, delim);
        if (tok == NULL) {
            continue;
        }

        entry.gvar = atoi(tok);

        tok = strtok(NULL, delim);
        if (tok == NULL) {
            continue;
        }

        entry.name = atoi(tok);

        tok = strtok(NULL, delim);
        if (tok == NULL) {
            continue;
        }

        entry.description = atoi(tok);

        HolodiskDescription* entries = (HolodiskDescription*)internal_realloc(gHolodiskDescriptions, sizeof(*gHolodiskDescriptions) * (gHolodisksCount + 1));
        if (entries == NULL) {
            goto err;
        }

        memcpy(&(entries[gHolodisksCount]), &entry, sizeof(*gHolodiskDescriptions));

        gHolodiskDescriptions = entries;
        gHolodisksCount++;
    }

    fileClose(stream);

    return 0;

err:

    fileClose(stream);

    return -1;
}

// 0x49A968
void holodiskFree()
{
    if (gHolodiskDescriptions != NULL) {
        internal_free(gHolodiskDescriptions);
        gHolodiskDescriptions = NULL;
    }

    gHolodisksCount = 0;
}
