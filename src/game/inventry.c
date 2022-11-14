#include "game/inventry.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "game/actions.h"
#include "game/anim.h"
#include "game/art.h"
#include "color.h"
#include "game/combat.h"
#include "game/combatai.h"
#include "core.h"
#include "game/critter.h"
#include "game/bmpdlog.h"
#include "debug.h"
#include "int/dialog.h"
#include "game/display.h"
#include "draw.h"
#include "game/game.h"
#include "game/gdialog.h"
#include "game/gmouse.h"
#include "game/gsound.h"
#include "game/intface.h"
#include "item.h"
#include "light.h"
#include "map.h"
#include "message.h"
#include "object.h"
#include "perk.h"
#include "proto.h"
#include "proto_instance.h"
#include "random.h"
#include "reaction.h"
#include "scripts.h"
#include "skill.h"
#include "stat.h"
#include "text_font.h"
#include "tile.h"
#include "window_manager.h"

#define INVENTORY_WINDOW_X 80
#define INVENTORY_WINDOW_Y 0

#define INVENTORY_TRADE_WINDOW_X 80
#define INVENTORY_TRADE_WINDOW_Y 290
#define INVENTORY_TRADE_WINDOW_WIDTH 480
#define INVENTORY_TRADE_WINDOW_HEIGHT 180

#define INVENTORY_LARGE_SLOT_WIDTH 90
#define INVENTORY_LARGE_SLOT_HEIGHT 61

#define INVENTORY_SLOT_WIDTH 64
#define INVENTORY_SLOT_HEIGHT 48

#define INVENTORY_LEFT_HAND_SLOT_X 154
#define INVENTORY_LEFT_HAND_SLOT_Y 286
#define INVENTORY_LEFT_HAND_SLOT_MAX_X (INVENTORY_LEFT_HAND_SLOT_X + INVENTORY_LARGE_SLOT_WIDTH)
#define INVENTORY_LEFT_HAND_SLOT_MAX_Y (INVENTORY_LEFT_HAND_SLOT_Y + INVENTORY_LARGE_SLOT_HEIGHT)

#define INVENTORY_RIGHT_HAND_SLOT_X 245
#define INVENTORY_RIGHT_HAND_SLOT_Y 286
#define INVENTORY_RIGHT_HAND_SLOT_MAX_X (INVENTORY_RIGHT_HAND_SLOT_X + INVENTORY_LARGE_SLOT_WIDTH)
#define INVENTORY_RIGHT_HAND_SLOT_MAX_Y (INVENTORY_RIGHT_HAND_SLOT_Y + INVENTORY_LARGE_SLOT_HEIGHT)

#define INVENTORY_ARMOR_SLOT_X 154
#define INVENTORY_ARMOR_SLOT_Y 183
#define INVENTORY_ARMOR_SLOT_MAX_X (INVENTORY_ARMOR_SLOT_X + INVENTORY_LARGE_SLOT_WIDTH)
#define INVENTORY_ARMOR_SLOT_MAX_Y (INVENTORY_ARMOR_SLOT_Y + INVENTORY_LARGE_SLOT_HEIGHT)

#define INVENTORY_TRADE_LEFT_SCROLLER_X 29
#define INVENTORY_TRADE_RIGHT_SCROLLER_X 395

#define INVENTORY_TRADE_INNER_LEFT_SCROLLER_X 165
#define INVENTORY_TRADE_INNER_RIGHT_SCROLLER_X 250

#define INVENTORY_TRADE_OUTER_SCROLLER_Y 35
#define INVENTORY_TRADE_INNER_SCROLLER_Y 20

#define INVENTORY_TRADE_LEFT_SCROLLER_TRACKING_X 165
#define INVENTORY_TRADE_LEFT_SCROLLER_TRACKING_Y 10
#define INVENTORY_TRADE_LEFT_SCROLLER_TRACKING_MAX_X (INVENTORY_TRADE_LEFT_SCROLLER_TRACKING_X + INVENTORY_SLOT_WIDTH)

#define INVENTORY_TRADE_INNER_LEFT_SCROLLER_TRACKING_X 0
#define INVENTORY_TRADE_INNER_LEFT_SCROLLER_TRACKING_Y 10
#define INVENTORY_TRADE_INNER_LEFT_SCROLLER_TRACKING_MAX_X (INVENTORY_TRADE_INNER_LEFT_SCROLLER_TRACKING_X + INVENTORY_SLOT_WIDTH)

#define INVENTORY_TRADE_INNER_RIGHT_SCROLLER_TRACKING_X 250
#define INVENTORY_TRADE_INNER_RIGHT_SCROLLER_TRACKING_Y 10
#define INVENTORY_TRADE_INNER_RIGHT_SCROLLER_TRACKING_MAX_X (INVENTORY_TRADE_INNER_RIGHT_SCROLLER_TRACKING_X + INVENTORY_SLOT_WIDTH)

#define INVENTORY_TRADE_RIGHT_SCROLLER_TRACKING_X 395
#define INVENTORY_TRADE_RIGHT_SCROLLER_TRACKING_Y 10
#define INVENTORY_TRADE_RIGHT_SCROLLER_TRACKING_MAX_X (INVENTORY_TRADE_RIGHT_SCROLLER_TRACKING_X + INVENTORY_SLOT_WIDTH)

#define INVENTORY_LOOT_LEFT_SCROLLER_X 176
#define INVENTORY_LOOT_LEFT_SCROLLER_Y 37
#define INVENTORY_LOOT_LEFT_SCROLLER_MAX_X (INVENTORY_LOOT_LEFT_SCROLLER_X + INVENTORY_SLOT_WIDTH)

#define INVENTORY_LOOT_RIGHT_SCROLLER_X 297
#define INVENTORY_LOOT_RIGHT_SCROLLER_Y 37
#define INVENTORY_LOOT_RIGHT_SCROLLER_MAX_X (INVENTORY_LOOT_RIGHT_SCROLLER_X + INVENTORY_SLOT_WIDTH)

#define INVENTORY_SCROLLER_X 44
#define INVENTORY_SCROLLER_Y 35
#define INVENTORY_SCROLLER_MAX_X (INVENTORY_SCROLLER_X + INVENTORY_SLOT_WIDTH)

#define INVENTORY_BODY_VIEW_WIDTH 60
#define INVENTORY_BODY_VIEW_HEIGHT 100

#define INVENTORY_PC_BODY_VIEW_X 176
#define INVENTORY_PC_BODY_VIEW_Y 37
#define INVENTORY_PC_BODY_VIEW_MAX_X (INVENTORY_PC_BODY_VIEW_X + INVENTORY_BODY_VIEW_WIDTH)
#define INVENTORY_PC_BODY_VIEW_MAX_Y (INVENTORY_PC_BODY_VIEW_Y + INVENTORY_BODY_VIEW_HEIGHT)

#define INVENTORY_LOOT_RIGHT_BODY_VIEW_X 422
#define INVENTORY_LOOT_RIGHT_BODY_VIEW_Y 35

#define INVENTORY_LOOT_LEFT_BODY_VIEW_X 44
#define INVENTORY_LOOT_LEFT_BODY_VIEW_Y 35

// NOTE: CE uses relative coordinates for hit testing for which coordinates
// above is enough. However RE requires separate sets of coordinates as it
// performs hit tests in screen coordinates.

#define INVENTORY_LEFT_HAND_SLOT_ABS_X (INVENTORY_WINDOW_X + INVENTORY_LEFT_HAND_SLOT_X)
#define INVENTORY_LEFT_HAND_SLOT_ABS_Y (INVENTORY_WINDOW_Y + INVENTORY_LEFT_HAND_SLOT_Y)
#define INVENTORY_LEFT_HAND_SLOT_ABS_MAX_X (INVENTORY_WINDOW_X + INVENTORY_LEFT_HAND_SLOT_MAX_X)
#define INVENTORY_LEFT_HAND_SLOT_ABS_MAX_Y (INVENTORY_WINDOW_Y + INVENTORY_LEFT_HAND_SLOT_MAX_Y)

#define INVENTORY_RIGHT_HAND_SLOT_ABS_X (INVENTORY_WINDOW_X + INVENTORY_RIGHT_HAND_SLOT_X)
#define INVENTORY_RIGHT_HAND_SLOT_ABS_Y (INVENTORY_WINDOW_Y + INVENTORY_RIGHT_HAND_SLOT_Y)
#define INVENTORY_RIGHT_HAND_SLOT_ABS_MAX_X (INVENTORY_WINDOW_X + INVENTORY_RIGHT_HAND_SLOT_MAX_X)
#define INVENTORY_RIGHT_HAND_SLOT_ABS_MAX_Y (INVENTORY_WINDOW_Y + INVENTORY_RIGHT_HAND_SLOT_MAX_Y)

#define INVENTORY_ARMOR_SLOT_ABS_X (INVENTORY_WINDOW_X + INVENTORY_ARMOR_SLOT_X)
#define INVENTORY_ARMOR_SLOT_ABS_Y (INVENTORY_WINDOW_Y + INVENTORY_ARMOR_SLOT_Y)
#define INVENTORY_ARMOR_SLOT_ABS_MAX_X (INVENTORY_WINDOW_X + INVENTORY_ARMOR_SLOT_MAX_X)
#define INVENTORY_ARMOR_SLOT_ABS_MAX_Y (INVENTORY_WINDOW_Y + INVENTORY_ARMOR_SLOT_MAX_Y)

#define INVENTORY_TRADE_LEFT_SCROLLER_TRACKING_ABS_X (INVENTORY_TRADE_WINDOW_X + INVENTORY_TRADE_LEFT_SCROLLER_TRACKING_X)
#define INVENTORY_TRADE_LEFT_SCROLLER_TRACKING_ABS_Y (INVENTORY_TRADE_WINDOW_Y + 10 + INVENTORY_TRADE_LEFT_SCROLLER_TRACKING_Y)
#define INVENTORY_TRADE_LEFT_SCROLLER_TRACKING_ABS_MAX_X (INVENTORY_TRADE_WINDOW_X + INVENTORY_TRADE_LEFT_SCROLLER_TRACKING_X + INVENTORY_SLOT_WIDTH)

#define INVENTORY_TRADE_INNER_LEFT_SCROLLER_TRACKING_ABS_X (INVENTORY_TRADE_WINDOW_X + INVENTORY_TRADE_INNER_LEFT_SCROLLER_TRACKING_X)
#define INVENTORY_TRADE_INNER_LEFT_SCROLLER_TRACKING_ABS_Y (INVENTORY_TRADE_WINDOW_Y + 10 + INVENTORY_TRADE_INNER_LEFT_SCROLLER_TRACKING_Y)
#define INVENTORY_TRADE_INNER_LEFT_SCROLLER_TRACKING_ABS_MAX_X (INVENTORY_TRADE_WINDOW_X + INVENTORY_TRADE_INNER_LEFT_SCROLLER_TRACKING_X + INVENTORY_SLOT_WIDTH)

#define INVENTORY_TRADE_INNER_RIGHT_SCROLLER_TRACKING_ABS_X (INVENTORY_TRADE_WINDOW_X + INVENTORY_TRADE_INNER_RIGHT_SCROLLER_TRACKING_X)
#define INVENTORY_TRADE_INNER_RIGHT_SCROLLER_TRACKING_ABS_Y (INVENTORY_TRADE_WINDOW_Y + 10 + INVENTORY_TRADE_INNER_RIGHT_SCROLLER_TRACKING_Y)
#define INVENTORY_TRADE_INNER_RIGHT_SCROLLER_TRACKING_ABS_MAX_X (INVENTORY_TRADE_WINDOW_X + INVENTORY_TRADE_INNER_RIGHT_SCROLLER_TRACKING_X + INVENTORY_SLOT_WIDTH)

#define INVENTORY_TRADE_RIGHT_SCROLLER_TRACKING_ABS_X (INVENTORY_TRADE_WINDOW_X + INVENTORY_TRADE_RIGHT_SCROLLER_TRACKING_X)
#define INVENTORY_TRADE_RIGHT_SCROLLER_TRACKING_ABS_Y (INVENTORY_TRADE_WINDOW_Y + 10 + INVENTORY_TRADE_RIGHT_SCROLLER_TRACKING_Y)
#define INVENTORY_TRADE_RIGHT_SCROLLER_TRACKING_ABS_MAX_X (INVENTORY_TRADE_WINDOW_X + INVENTORY_TRADE_RIGHT_SCROLLER_TRACKING_X + INVENTORY_SLOT_WIDTH)

#define INVENTORY_LOOT_LEFT_SCROLLER_ABS_X (INVENTORY_WINDOW_X + INVENTORY_LOOT_LEFT_SCROLLER_X)
#define INVENTORY_LOOT_LEFT_SCROLLER_ABS_Y (INVENTORY_WINDOW_Y + INVENTORY_LOOT_LEFT_SCROLLER_Y)
#define INVENTORY_LOOT_LEFT_SCROLLER_ABS_MAX_X (INVENTORY_WINDOW_X + INVENTORY_LOOT_LEFT_SCROLLER_MAX_X)

#define INVENTORY_LOOT_RIGHT_SCROLLER_ABS_X (INVENTORY_WINDOW_X + INVENTORY_LOOT_RIGHT_SCROLLER_X)
#define INVENTORY_LOOT_RIGHT_SCROLLER_ABS_Y (INVENTORY_WINDOW_Y + INVENTORY_LOOT_RIGHT_SCROLLER_Y)
#define INVENTORY_LOOT_RIGHT_SCROLLER_ABS_MAX_X (INVENTORY_WINDOW_X + INVENTORY_LOOT_RIGHT_SCROLLER_MAX_X)

#define INVENTORY_SCROLLER_ABS_X (INVENTORY_WINDOW_X + INVENTORY_SCROLLER_X)
#define INVENTORY_SCROLLER_ABS_Y (INVENTORY_WINDOW_Y + INVENTORY_SCROLLER_Y)
#define INVENTORY_SCROLLER_ABS_MAX_X (INVENTORY_WINDOW_X + INVENTORY_SCROLLER_MAX_X)

#define INVENTORY_PC_BODY_VIEW_ABS_X (INVENTORY_WINDOW_X + INVENTORY_PC_BODY_VIEW_X)
#define INVENTORY_PC_BODY_VIEW_ABS_Y (INVENTORY_WINDOW_Y + INVENTORY_PC_BODY_VIEW_Y)
#define INVENTORY_PC_BODY_VIEW_ABS_MAX_X (INVENTORY_WINDOW_X + INVENTORY_PC_BODY_VIEW_MAX_X)
#define INVENTORY_PC_BODY_VIEW_ABS_MAX_Y (INVENTORY_WINDOW_Y + INVENTORY_PC_BODY_VIEW_MAX_Y)

#define INVENTORY_NORMAL_WINDOW_PC_ROTATION_DELAY (1000U / ROTATION_COUNT)

typedef void(InventoryPrintItemDescriptionHandler)(char* string);

typedef enum InventoryArrowFrm {
    INVENTORY_ARROW_FRM_LEFT_ARROW_UP,
    INVENTORY_ARROW_FRM_LEFT_ARROW_DOWN,
    INVENTORY_ARROW_FRM_RIGHT_ARROW_UP,
    INVENTORY_ARROW_FRM_RIGHT_ARROW_DOWN,
    INVENTORY_ARROW_FRM_COUNT,
} InventoryArrowFrm;

typedef struct InventoryWindowConfiguration {
    int field_0; // artId
    int width;
    int height;
    int x;
    int y;
} InventoryWindowDescription;

typedef struct InventoryCursorData {
    Art* frm;
    unsigned char* frmData;
    int width;
    int height;
    int offsetX;
    int offsetY;
    CacheEntry* frmHandle;
} InventoryCursorData;

static int inventry_msg_load();
static int inventry_msg_unload();
static void display_inventory_info(Object* item, int quantity, unsigned char* dest, int pitch, bool a5);
static void inven_update_lighting(Object* a1);
static int barter_compute_value(Object* a1, Object* a2);
static int barter_attempt_transaction(Object* a1, Object* a2, Object* a3, Object* a4);
static void barter_move_inventory(Object* a1, int quantity, int a3, int a4, Object* a5, Object* a6, bool a7);
static void barter_move_from_table_inventory(Object* a1, int quantity, int a3, Object* a4, Object* a5, bool a6);
static void display_table_inventories(int win, Object* a2, Object* a3, int a4);
static int do_move_timer(int inventoryWindowType, Object* item, int a3);
static int setup_move_timer_win(int inventoryWindowType, Object* item);
static int exit_move_timer_win(int inventoryWindowType);

// The number of items to show in scroller.
//
// 0x519054
static int inven_cur_disp = 6;

// 0x519058
static Object* inven_dude = NULL;

// Probably fid of armor to display in inventory dialog.
//
// 0x51905C
static int inven_pid = -1;

// 0x519060
static bool inven_is_initialized = false;

// 0x519064
static int inven_display_msg_line = 1;

// 0x519068
static InventoryWindowDescription iscr_data[INVENTORY_WINDOW_TYPE_COUNT] = {
    { 48, 499, 377, 80, 0 },
    { 113, 292, 376, 80, 0 },
    { 114, 537, 376, 80, 0 },
    { 111, 480, 180, 80, 290 },
    { 305, 259, 162, 140, 80 },
    { 305, 259, 162, 140, 80 },
};

// 0x5190E0
static bool dropped_explosive = false;

// 0x5190E4
static int inven_scroll_up_bid = -1;

// 0x5190E8
static int inven_scroll_dn_bid = -1;

// 0x5190EC
static int loot_scroll_up_bid = -1;

// 0x5190F0
static int loot_scroll_dn_bid = -1;

// 0x59E79C
static CacheEntry* mt_key[8];

// 0x59E7BC
CacheEntry* ikey[OFF_59E7BC_COUNT];

// 0x59E7EC
static int target_stack_offset[10];

// inventory.msg
//
// 0x59E814
static MessageList inventry_message_file;

// 0x59E81C
static Object* target_stack[10];

// 0x59E844
static int stack_offset[10];

// 0x59E86C
static Object* stack[10];

// 0x59E894
static int mt_wid;

// 0x59E898
static int barter_mod;

// 0x59E89C
static int btable_offset;

// 0x59E8A0
static int ptable_offset;

// 0x59E8A4
static Inventory* ptable_pud;

// 0x59E8A8
static InventoryCursorData imdata[INVENTORY_WINDOW_CURSOR_COUNT];

// 0x59E934
static Object* ptable;

// 0x59E938
static InventoryPrintItemDescriptionHandler* display_msg;

// 0x59E93C
static int im_value;

// 0x59E940
static int immode;

// 0x59E944
static Object* btable;

// 0x59E948
static int target_curr_stack;

// 0x59E94C
static Inventory* btable_pud;

// 0x59E950
static bool inven_ui_was_disabled;

// 0x59E954
static Object* i_worn;

// 0x59E958
static Object* i_lhand;

// Rotating character's fid.
//
// 0x59E95C
static int i_fid;

// 0x59E960
static Inventory* pud;

// 0x59E964
static int i_wid;

// item2
// 0x59E968
static Object* i_rhand;

// 0x59E96C
static int curr_stack;

// 0x59E970
static int i_wid_max_y;

// 0x59E974
static int i_wid_max_x;

// 0x59E978
static Inventory* target_pud;

// 0x59E97C
static int barter_back_win;

// NOTE: Unused.
//
// 0x46E718
void inven_set_dude(Object* obj, int pid)
{
    inven_dude = obj;
    inven_pid = pid;
}

// 0x46E724
void inven_reset_dude()
{
    inven_dude = gDude;
    inven_pid = 0x1000000;
}

// 0x46E73C
static int inventry_msg_load()
{
    char path[MAX_PATH];

    if (!messageListInit(&inventry_message_file))
        return -1;

    sprintf(path, "%s%s", msg_path, "inventry.msg");
    if (!messageListLoad(&inventry_message_file, path))
        return -1;

    return 0;
}

// 0x46E7A0
static int inventry_msg_unload()
{
    messageListFree(&inventry_message_file);
    return 0;
}

// 0x46E7B0
void handle_inventory()
{
    if (isInCombat()) {
        if (combat_whose_turn() != inven_dude) {
            return;
        }
    }

    if (inven_init() == -1) {
        return;
    }

    if (isInCombat()) {
        if (inven_dude == gDude) {
            int actionPointsRequired = 4 - 2 * perkGetRank(inven_dude, PERK_QUICK_POCKETS);
            if (actionPointsRequired > 0 && actionPointsRequired > gDude->data.critter.combat.ap) {
                // You don't have enough action points to use inventory
                MessageListItem messageListItem;
                messageListItem.num = 19;
                if (messageListGetItem(&inventry_message_file, &messageListItem)) {
                    display_print(messageListItem.text);
                }

                // NOTE: Uninline.
                inven_exit();

                return;
            }

            if (actionPointsRequired > 0) {
                if (actionPointsRequired > gDude->data.critter.combat.ap) {
                    gDude->data.critter.combat.ap = 0;
                } else {
                    gDude->data.critter.combat.ap -= actionPointsRequired;
                }
                intface_update_move_points(gDude->data.critter.combat.ap, combat_free_move);
            }
        }
    }

    Object* oldArmor = inven_worn(inven_dude);
    bool isoWasEnabled = setup_inventory(INVENTORY_WINDOW_TYPE_NORMAL);
    register_clear(inven_dude);
    display_stats();
    display_inventory(stack_offset[curr_stack], -1, INVENTORY_WINDOW_TYPE_NORMAL);
    inven_set_mouse(INVENTORY_WINDOW_CURSOR_HAND);

    for (;;) {
        int keyCode = _get_input();

        if (keyCode == KEY_ESCAPE) {
            break;
        }

        if (game_user_wants_to_quit != 0) {
            break;
        }

        display_body(-1, INVENTORY_WINDOW_TYPE_NORMAL);

        if (game_state() == GAME_STATE_5) {
            break;
        }

        if (keyCode == KEY_CTRL_Q || keyCode == KEY_CTRL_X) {
            game_quit_with_confirm();
        } else if (keyCode == KEY_HOME) {
            stack_offset[curr_stack] = 0;
            display_inventory(0, -1, INVENTORY_WINDOW_TYPE_NORMAL);
        } else if (keyCode == KEY_ARROW_UP) {
            if (stack_offset[curr_stack] > 0) {
                stack_offset[curr_stack] -= 1;
                display_inventory(stack_offset[curr_stack], -1, INVENTORY_WINDOW_TYPE_NORMAL);
            }
        } else if (keyCode == KEY_PAGE_UP) {
            stack_offset[curr_stack] -= inven_cur_disp;
            if (stack_offset[curr_stack] < 0) {
                stack_offset[curr_stack] = 0;
            }
            display_inventory(stack_offset[curr_stack], -1, INVENTORY_WINDOW_TYPE_NORMAL);
        } else if (keyCode == KEY_END) {
            stack_offset[curr_stack] = pud->length - inven_cur_disp;
            if (stack_offset[curr_stack] < 0) {
                stack_offset[curr_stack] = 0;
            }
            display_inventory(stack_offset[curr_stack], -1, INVENTORY_WINDOW_TYPE_NORMAL);
        } else if (keyCode == KEY_ARROW_DOWN) {
            if (inven_cur_disp + stack_offset[curr_stack] < pud->length) {
                stack_offset[curr_stack] += 1;
                display_inventory(stack_offset[curr_stack], -1, INVENTORY_WINDOW_TYPE_NORMAL);
            }
        } else if (keyCode == KEY_PAGE_DOWN) {
            int v12 = inven_cur_disp + stack_offset[curr_stack];
            int v13 = v12 + inven_cur_disp;
            stack_offset[curr_stack] = v12;
            int v14 = pud->length;
            if (v13 >= pud->length) {
                int v15 = v14 - inven_cur_disp;
                stack_offset[curr_stack] = v14 - inven_cur_disp;
                if (v15 < 0) {
                    stack_offset[curr_stack] = 0;
                }
            }
            display_inventory(stack_offset[curr_stack], -1, INVENTORY_WINDOW_TYPE_NORMAL);
        } else if (keyCode == 2500) {
            container_exit(keyCode, INVENTORY_WINDOW_TYPE_NORMAL);
        } else {
            if ((mouse_get_buttons() & MOUSE_EVENT_RIGHT_BUTTON_DOWN) != 0) {
                if (immode == INVENTORY_WINDOW_CURSOR_HAND) {
                    inven_set_mouse(INVENTORY_WINDOW_CURSOR_ARROW);
                } else if (immode == INVENTORY_WINDOW_CURSOR_ARROW) {
                    inven_set_mouse(INVENTORY_WINDOW_CURSOR_HAND);
                    display_stats();
                    win_draw(i_wid);
                }
            } else if ((mouse_get_buttons() & MOUSE_EVENT_LEFT_BUTTON_DOWN) != 0) {
                if (keyCode >= 1000 && keyCode <= 1008) {
                    if (immode == INVENTORY_WINDOW_CURSOR_ARROW) {
                        inven_action_cursor(keyCode, INVENTORY_WINDOW_TYPE_NORMAL);
                    } else {
                        inven_pickup(keyCode, stack_offset[curr_stack]);
                    }
                }
            }
        }
    }

    inven_dude = stack[0];
    adjust_fid();

    if (inven_dude == gDude) {
        Rect rect;
        objectSetFid(inven_dude, i_fid, &rect);
        tileWindowRefreshRect(&rect, inven_dude->elevation);
    }

    Object* newArmor = inven_worn(inven_dude);
    if (inven_dude == gDude) {
        if (oldArmor != newArmor) {
            intface_update_ac(true);
        }
    }

    exit_inventory(isoWasEnabled);

    // NOTE: Uninline.
    inven_exit();

    if (inven_dude == gDude) {
        intface_update_items(false, INTERFACE_ITEM_ACTION_DEFAULT, INTERFACE_ITEM_ACTION_DEFAULT);
    }
}

// 0x46EC90
bool setup_inventory(int inventoryWindowType)
{
    dropped_explosive = 0;
    curr_stack = 0;
    stack_offset[0] = 0;
    inven_cur_disp = 6;
    pud = &(inven_dude->data.inventory);
    stack[0] = inven_dude;

    if (inventoryWindowType <= INVENTORY_WINDOW_TYPE_LOOT) {
        InventoryWindowDescription* windowDescription = &(iscr_data[inventoryWindowType]);
        int inventoryWindowX = INVENTORY_WINDOW_X;
        int inventoryWindowY = INVENTORY_WINDOW_Y;
        i_wid = windowCreate(inventoryWindowX,
            inventoryWindowY,
            windowDescription->width,
            windowDescription->height,
            257,
            WINDOW_FLAG_0x10 | WINDOW_FLAG_0x04);
        i_wid_max_x = windowDescription->width + inventoryWindowX;
        i_wid_max_y = windowDescription->height + inventoryWindowY;

        unsigned char* dest = windowGetBuffer(i_wid);
        int backgroundFid = art_id(OBJ_TYPE_INTERFACE, windowDescription->field_0, 0, 0, 0);

        CacheEntry* backgroundFrmHandle;
        unsigned char* backgroundFrmData = art_ptr_lock_data(backgroundFid, 0, 0, &backgroundFrmHandle);
        if (backgroundFrmData != NULL) {
            blitBufferToBuffer(backgroundFrmData, windowDescription->width, windowDescription->height, windowDescription->width, dest, windowDescription->width);
            art_ptr_unlock(backgroundFrmHandle);
        }

        display_msg = display_print;
    } else if (inventoryWindowType == INVENTORY_WINDOW_TYPE_TRADE) {
        if (barter_back_win == -1) {
            exit(1);
        }

        inven_cur_disp = 3;

        int tradeWindowX = INVENTORY_TRADE_WINDOW_X;
        int tradeWindowY = INVENTORY_TRADE_WINDOW_Y;
        i_wid = windowCreate(tradeWindowX, tradeWindowY, INVENTORY_TRADE_WINDOW_WIDTH, INVENTORY_TRADE_WINDOW_HEIGHT, 257, 0);
        i_wid_max_x = tradeWindowX + INVENTORY_TRADE_WINDOW_WIDTH;
        i_wid_max_y = tradeWindowY + INVENTORY_TRADE_WINDOW_HEIGHT;

        unsigned char* dest = windowGetBuffer(i_wid);
        unsigned char* src = windowGetBuffer(barter_back_win);
        blitBufferToBuffer(src + INVENTORY_TRADE_WINDOW_X, INVENTORY_TRADE_WINDOW_WIDTH, INVENTORY_TRADE_WINDOW_HEIGHT, _scr_size.right - _scr_size.left + 1, dest, INVENTORY_TRADE_WINDOW_WIDTH);

        display_msg = gdialogDisplayMsg;
    }

    if (inventoryWindowType == INVENTORY_WINDOW_TYPE_LOOT) {
        // Create invsibile buttons representing character's inventory item
        // slots.
        for (int index = 0; index < inven_cur_disp; index++) {
            int btn = buttonCreate(i_wid, INVENTORY_LOOT_LEFT_SCROLLER_X, INVENTORY_SLOT_HEIGHT * (inven_cur_disp - index - 1) + INVENTORY_LOOT_LEFT_SCROLLER_Y, INVENTORY_SLOT_WIDTH, INVENTORY_SLOT_HEIGHT, 999 + inven_cur_disp - index, -1, 999 + inven_cur_disp - index, -1, NULL, NULL, NULL, 0);
            if (btn != -1) {
                buttonSetMouseCallbacks(btn, inven_hover_on, inven_hover_off, NULL, NULL);
            }
        }

        int eventCode = 2005;
        int y = INVENTORY_SLOT_HEIGHT * 5 + INVENTORY_LOOT_LEFT_SCROLLER_Y;

        // Create invisible buttons representing container's inventory item
        // slots. For unknown reason it loops backwards and it's size is
        // hardcoded at 6 items.
        //
        // Original code is slightly different. It loops until y reaches -11,
        // which is a bit awkward for a loop. Probably result of some
        // optimization.
        for (int index = 0; index < 6; index++) {
            int btn = buttonCreate(i_wid, INVENTORY_LOOT_RIGHT_SCROLLER_X, y, INVENTORY_SLOT_WIDTH, INVENTORY_SLOT_HEIGHT, eventCode, -1, eventCode, -1, NULL, NULL, NULL, 0);
            if (btn != -1) {
                buttonSetMouseCallbacks(btn, inven_hover_on, inven_hover_off, NULL, NULL);
            }

            eventCode -= 1;
            y -= INVENTORY_SLOT_HEIGHT;
        }
    } else if (inventoryWindowType == INVENTORY_WINDOW_TYPE_TRADE) {
        int y1 = INVENTORY_TRADE_OUTER_SCROLLER_Y;
        int y2 = INVENTORY_TRADE_INNER_SCROLLER_Y;

        for (int index = 0; index < inven_cur_disp; index++) {
            int btn;

            // Invsibile button representing left inventory slot.
            btn = buttonCreate(i_wid, INVENTORY_TRADE_LEFT_SCROLLER_X, y1, INVENTORY_SLOT_WIDTH, INVENTORY_SLOT_HEIGHT, 1000 + index, -1, 1000 + index, -1, NULL, NULL, NULL, 0);
            if (btn != -1) {
                buttonSetMouseCallbacks(btn, inven_hover_on, inven_hover_off, NULL, NULL);
            }

            // Invisible button representing right inventory slot.
            btn = buttonCreate(i_wid, INVENTORY_TRADE_RIGHT_SCROLLER_X, y1, INVENTORY_SLOT_WIDTH, INVENTORY_SLOT_HEIGHT, 2000 + index, -1, 2000 + index, -1, NULL, NULL, NULL, 0);
            if (btn != -1) {
                buttonSetMouseCallbacks(btn, inven_hover_on, inven_hover_off, NULL, NULL);
            }

            // Invisible button representing left suggested slot.
            btn = buttonCreate(i_wid, INVENTORY_TRADE_INNER_LEFT_SCROLLER_X, y2, INVENTORY_SLOT_WIDTH, INVENTORY_SLOT_HEIGHT, 2300 + index, -1, 2300 + index, -1, NULL, NULL, NULL, 0);
            if (btn != -1) {
                buttonSetMouseCallbacks(btn, inven_hover_on, inven_hover_off, NULL, NULL);
            }

            // Invisible button representing right suggested slot.
            btn = buttonCreate(i_wid, INVENTORY_TRADE_INNER_RIGHT_SCROLLER_X, y2, INVENTORY_SLOT_WIDTH, INVENTORY_SLOT_HEIGHT, 2400 + index, -1, 2400 + index, -1, NULL, NULL, NULL, 0);
            if (btn != -1) {
                buttonSetMouseCallbacks(btn, inven_hover_on, inven_hover_off, NULL, NULL);
            }

            y1 += INVENTORY_SLOT_HEIGHT;
            y2 += INVENTORY_SLOT_HEIGHT;
        }
    } else {
        // Create invisible buttons representing item slots.
        for (int index = 0; index < inven_cur_disp; index++) {
            int btn = buttonCreate(i_wid,
                INVENTORY_SCROLLER_X,
                INVENTORY_SLOT_HEIGHT * (inven_cur_disp - index - 1) + INVENTORY_SCROLLER_Y,
                INVENTORY_SLOT_WIDTH,
                INVENTORY_SLOT_HEIGHT,
                999 + inven_cur_disp - index,
                -1,
                999 + inven_cur_disp - index,
                -1,
                NULL,
                NULL,
                NULL,
                0);
            if (btn != -1) {
                buttonSetMouseCallbacks(btn, inven_hover_on, inven_hover_off, NULL, NULL);
            }
        }
    }

    if (inventoryWindowType == INVENTORY_WINDOW_TYPE_NORMAL) {
        int btn;

        // Item2 slot
        btn = buttonCreate(i_wid, INVENTORY_RIGHT_HAND_SLOT_X, INVENTORY_RIGHT_HAND_SLOT_Y, INVENTORY_LARGE_SLOT_WIDTH, INVENTORY_LARGE_SLOT_HEIGHT, 1006, -1, 1006, -1, NULL, NULL, NULL, 0);
        if (btn != -1) {
            buttonSetMouseCallbacks(btn, inven_hover_on, inven_hover_off, NULL, NULL);
        }

        // Item1 slot
        btn = buttonCreate(i_wid, INVENTORY_LEFT_HAND_SLOT_X, INVENTORY_LEFT_HAND_SLOT_Y, INVENTORY_LARGE_SLOT_WIDTH, INVENTORY_LARGE_SLOT_HEIGHT, 1007, -1, 1007, -1, NULL, NULL, NULL, 0);
        if (btn != -1) {
            buttonSetMouseCallbacks(btn, inven_hover_on, inven_hover_off, NULL, NULL);
        }

        // Armor slot
        btn = buttonCreate(i_wid, INVENTORY_ARMOR_SLOT_X, INVENTORY_ARMOR_SLOT_Y, INVENTORY_LARGE_SLOT_WIDTH, INVENTORY_LARGE_SLOT_HEIGHT, 1008, -1, 1008, -1, NULL, NULL, NULL, 0);
        if (btn != -1) {
            buttonSetMouseCallbacks(btn, inven_hover_on, inven_hover_off, NULL, NULL);
        }
    }

    memset(ikey, 0, sizeof(ikey));

    int fid;
    int btn;
    unsigned char* buttonUpData;
    unsigned char* buttonDownData;
    unsigned char* buttonDisabledData;

    fid = art_id(OBJ_TYPE_INTERFACE, 8, 0, 0, 0);
    buttonUpData = art_ptr_lock_data(fid, 0, 0, &(ikey[0]));

    fid = art_id(OBJ_TYPE_INTERFACE, 9, 0, 0, 0);
    buttonDownData = art_ptr_lock_data(fid, 0, 0, &(ikey[1]));

    if (buttonUpData != NULL && buttonDownData != NULL) {
        btn = -1;
        switch (inventoryWindowType) {
        case INVENTORY_WINDOW_TYPE_NORMAL:
            // Done button
            btn = buttonCreate(i_wid, 437, 329, 15, 16, -1, -1, -1, KEY_ESCAPE, buttonUpData, buttonDownData, NULL, BUTTON_FLAG_TRANSPARENT);
            break;
        case INVENTORY_WINDOW_TYPE_USE_ITEM_ON:
            // Cancel button
            btn = buttonCreate(i_wid, 233, 328, 15, 16, -1, -1, -1, KEY_ESCAPE, buttonUpData, buttonDownData, NULL, BUTTON_FLAG_TRANSPARENT);
            break;
        case INVENTORY_WINDOW_TYPE_LOOT:
            // Done button
            btn = buttonCreate(i_wid, 476, 331, 15, 16, -1, -1, -1, KEY_ESCAPE, buttonUpData, buttonDownData, NULL, BUTTON_FLAG_TRANSPARENT);
            break;
        }

        if (btn != -1) {
            buttonSetCallbacks(btn, gsound_red_butt_press, gsound_red_butt_release);
        }
    }

    if (inventoryWindowType == INVENTORY_WINDOW_TYPE_TRADE) {
        // TODO: Figure out why it building fid in chain.

        // Large arrow up (normal).
        fid = art_id(OBJ_TYPE_INTERFACE, 100, 0, 0, 0);
        fid = art_id(OBJ_TYPE_INTERFACE, fid, 0, 0, 0);
        buttonUpData = art_ptr_lock_data(fid, 0, 0, &(ikey[2]));

        // Large arrow up (pressed).
        fid = art_id(OBJ_TYPE_INTERFACE, 101, 0, 0, 0);
        fid = art_id(OBJ_TYPE_INTERFACE, fid, 0, 0, 0);
        buttonDownData = art_ptr_lock_data(fid, 0, 0, &(ikey[3]));

        if (buttonUpData != NULL && buttonDownData != NULL) {
            // Left inventory up button.
            btn = buttonCreate(i_wid, 109, 56, 23, 24, -1, -1, KEY_ARROW_UP, -1, buttonUpData, buttonDownData, NULL, 0);
            if (btn != -1) {
                buttonSetCallbacks(btn, gsound_red_butt_press, gsound_red_butt_release);
            }

            // Right inventory up button.
            btn = buttonCreate(i_wid, 342, 56, 23, 24, -1, -1, KEY_CTRL_ARROW_UP, -1, buttonUpData, buttonDownData, NULL, 0);
            if (btn != -1) {
                buttonSetCallbacks(btn, gsound_red_butt_press, gsound_red_butt_release);
            }
        }
    } else {
        // Large up arrow (normal).
        fid = art_id(OBJ_TYPE_INTERFACE, 49, 0, 0, 0);
        buttonUpData = art_ptr_lock_data(fid, 0, 0, &(ikey[2]));

        // Large up arrow (pressed).
        fid = art_id(OBJ_TYPE_INTERFACE, 50, 0, 0, 0);
        buttonDownData = art_ptr_lock_data(fid, 0, 0, &(ikey[3]));

        // Large up arrow (disabled).
        fid = art_id(OBJ_TYPE_INTERFACE, 53, 0, 0, 0);
        buttonDisabledData = art_ptr_lock_data(fid, 0, 0, &(ikey[4]));

        if (buttonUpData != NULL && buttonDownData != NULL && buttonDisabledData != NULL) {
            if (inventoryWindowType != INVENTORY_WINDOW_TYPE_TRADE) {
                // Left inventory up button.
                inven_scroll_up_bid = buttonCreate(i_wid, 128, 39, 22, 23, -1, -1, KEY_ARROW_UP, -1, buttonUpData, buttonDownData, NULL, 0);
                if (inven_scroll_up_bid != -1) {
                    _win_register_button_disable(inven_scroll_up_bid, buttonDisabledData, buttonDisabledData, buttonDisabledData);
                    buttonSetCallbacks(inven_scroll_up_bid, gsound_red_butt_press, gsound_red_butt_release);
                    buttonDisable(inven_scroll_up_bid);
                }
            }

            if (inventoryWindowType == INVENTORY_WINDOW_TYPE_LOOT) {
                // Right inventory up button.
                loot_scroll_up_bid = buttonCreate(i_wid, 379, 39, 22, 23, -1, -1, KEY_CTRL_ARROW_UP, -1, buttonUpData, buttonDownData, NULL, 0);
                if (loot_scroll_up_bid != -1) {
                    _win_register_button_disable(loot_scroll_up_bid, buttonDisabledData, buttonDisabledData, buttonDisabledData);
                    buttonSetCallbacks(loot_scroll_up_bid, gsound_red_butt_press, gsound_red_butt_release);
                    buttonDisable(loot_scroll_up_bid);
                }
            }
        }
    }

    if (inventoryWindowType == INVENTORY_WINDOW_TYPE_TRADE) {
        // Large dialog down button (normal)
        fid = art_id(OBJ_TYPE_INTERFACE, 93, 0, 0, 0);
        fid = art_id(OBJ_TYPE_INTERFACE, fid, 0, 0, 0);
        buttonUpData = art_ptr_lock_data(fid, 0, 0, &(ikey[5]));

        // Dialog down button (pressed)
        fid = art_id(OBJ_TYPE_INTERFACE, 94, 0, 0, 0);
        fid = art_id(OBJ_TYPE_INTERFACE, fid, 0, 0, 0);
        buttonDownData = art_ptr_lock_data(fid, 0, 0, &(ikey[6]));

        if (buttonUpData != NULL && buttonDownData != NULL) {
            // Left inventory down button.
            btn = buttonCreate(i_wid, 109, 82, 24, 25, -1, -1, KEY_ARROW_DOWN, -1, buttonUpData, buttonDownData, NULL, 0);
            if (btn != -1) {
                buttonSetCallbacks(btn, gsound_red_butt_press, gsound_red_butt_release);
            }

            // Right inventory down button
            btn = buttonCreate(i_wid, 342, 82, 24, 25, -1, -1, KEY_CTRL_ARROW_DOWN, -1, buttonUpData, buttonDownData, NULL, 0);
            if (btn != -1) {
                buttonSetCallbacks(btn, gsound_red_butt_press, gsound_red_butt_release);
            }

            // Invisible button representing left character.
            buttonCreate(barter_back_win, 15, 25, INVENTORY_BODY_VIEW_WIDTH, INVENTORY_BODY_VIEW_HEIGHT, -1, -1, 2500, -1, NULL, NULL, NULL, 0);

            // Invisible button representing right character.
            buttonCreate(barter_back_win, 560, 25, INVENTORY_BODY_VIEW_WIDTH, INVENTORY_BODY_VIEW_HEIGHT, -1, -1, 2501, -1, NULL, NULL, NULL, 0);
        }
    } else {
        // Large arrow down (normal).
        fid = art_id(OBJ_TYPE_INTERFACE, 51, 0, 0, 0);
        buttonUpData = art_ptr_lock_data(fid, 0, 0, &(ikey[5]));

        // Large arrow down (pressed).
        fid = art_id(OBJ_TYPE_INTERFACE, 52, 0, 0, 0);
        buttonDownData = art_ptr_lock_data(fid, 0, 0, &(ikey[6]));

        // Large arrow down (disabled).
        fid = art_id(OBJ_TYPE_INTERFACE, 54, 0, 0, 0);
        buttonDisabledData = art_ptr_lock_data(fid, 0, 0, &(ikey[7]));

        if (buttonUpData != NULL && buttonDownData != NULL && buttonDisabledData != NULL) {
            // Left inventory down button.
            inven_scroll_dn_bid = buttonCreate(i_wid, 128, 62, 22, 23, -1, -1, KEY_ARROW_DOWN, -1, buttonUpData, buttonDownData, NULL, 0);
            buttonSetCallbacks(inven_scroll_dn_bid, gsound_red_butt_press, gsound_red_butt_release);
            _win_register_button_disable(inven_scroll_dn_bid, buttonDisabledData, buttonDisabledData, buttonDisabledData);
            buttonDisable(inven_scroll_dn_bid);

            if (inventoryWindowType == INVENTORY_WINDOW_TYPE_LOOT) {
                // Invisible button representing left character.
                buttonCreate(i_wid, INVENTORY_LOOT_LEFT_BODY_VIEW_X, INVENTORY_LOOT_LEFT_BODY_VIEW_Y, INVENTORY_BODY_VIEW_WIDTH, INVENTORY_BODY_VIEW_HEIGHT, -1, -1, 2500, -1, NULL, NULL, NULL, 0);

                // Right inventory down button.
                loot_scroll_dn_bid = buttonCreate(i_wid, 379, 62, 22, 23, -1, -1, KEY_CTRL_ARROW_DOWN, -1, buttonUpData, buttonDownData, 0, 0);
                if (loot_scroll_dn_bid != -1) {
                    buttonSetCallbacks(loot_scroll_dn_bid, gsound_red_butt_press, gsound_red_butt_release);
                    _win_register_button_disable(loot_scroll_dn_bid, buttonDisabledData, buttonDisabledData, buttonDisabledData);
                    buttonDisable(loot_scroll_dn_bid);
                }

                // Invisible button representing right character.
                buttonCreate(i_wid, INVENTORY_LOOT_RIGHT_BODY_VIEW_X, INVENTORY_LOOT_RIGHT_BODY_VIEW_Y, INVENTORY_BODY_VIEW_WIDTH, INVENTORY_BODY_VIEW_HEIGHT, -1, -1, 2501, -1, NULL, NULL, NULL, 0);
            } else {
                // Invisible button representing character (in inventory and use on dialogs).
                buttonCreate(i_wid, INVENTORY_PC_BODY_VIEW_X, INVENTORY_PC_BODY_VIEW_Y, INVENTORY_BODY_VIEW_WIDTH, INVENTORY_BODY_VIEW_HEIGHT, -1, -1, 2500, -1, NULL, NULL, NULL, 0);
            }
        }
    }

    if (inventoryWindowType != INVENTORY_WINDOW_TYPE_TRADE) {
        if (inventoryWindowType == INVENTORY_WINDOW_TYPE_LOOT) {
            if (!_gIsSteal) {
                // Take all button (normal)
                fid = art_id(OBJ_TYPE_INTERFACE, 436, 0, 0, 0);
                buttonUpData = art_ptr_lock_data(fid, 0, 0, &(ikey[8]));

                // Take all button (pressed)
                fid = art_id(OBJ_TYPE_INTERFACE, 437, 0, 0, 0);
                buttonDownData = art_ptr_lock_data(fid, 0, 0, &(ikey[9]));

                if (buttonUpData != NULL && buttonDownData != NULL) {
                    // Take all button.
                    btn = buttonCreate(i_wid, 432, 204, 39, 41, -1, -1, KEY_UPPERCASE_A, -1, buttonUpData, buttonDownData, NULL, 0);
                    if (btn != -1) {
                        buttonSetCallbacks(btn, gsound_red_butt_press, gsound_red_butt_release);
                    }
                }
            }
        }
    } else {
        // Inventory button up (normal)
        fid = art_id(OBJ_TYPE_INTERFACE, 49, 0, 0, 0);
        buttonUpData = art_ptr_lock_data(fid, 0, 0, &(ikey[8]));

        // Inventory button up (pressed)
        fid = art_id(OBJ_TYPE_INTERFACE, 50, 0, 0, 0);
        buttonDownData = art_ptr_lock_data(fid, 0, 0, &(ikey[9]));

        if (buttonUpData != NULL && buttonDownData != NULL) {
            // Left offered inventory up button.
            btn = buttonCreate(i_wid, 128, 113, 22, 23, -1, -1, KEY_PAGE_UP, -1, buttonUpData, buttonDownData, NULL, 0);
            if (btn != -1) {
                buttonSetCallbacks(btn, gsound_red_butt_press, gsound_red_butt_release);
            }

            // Right offered inventory up button.
            btn = buttonCreate(i_wid, 333, 113, 22, 23, -1, -1, KEY_CTRL_PAGE_UP, -1, buttonUpData, buttonDownData, NULL, 0);
            if (btn != -1) {
                buttonSetCallbacks(btn, gsound_red_butt_press, gsound_red_butt_release);
            }
        }

        // Inventory button down (normal)
        fid = art_id(OBJ_TYPE_INTERFACE, 51, 0, 0, 0);
        buttonUpData = art_ptr_lock_data(fid, 0, 0, &(ikey[8]));

        // Inventory button down (pressed).
        fid = art_id(OBJ_TYPE_INTERFACE, 52, 0, 0, 0);
        buttonDownData = art_ptr_lock_data(fid, 0, 0, &(ikey[9]));

        if (buttonUpData != NULL && buttonDownData != NULL) {
            // Left offered inventory down button.
            btn = buttonCreate(i_wid, 128, 136, 22, 23, -1, -1, KEY_PAGE_DOWN, -1, buttonUpData, buttonDownData, NULL, 0);
            if (btn != -1) {
                buttonSetCallbacks(btn, gsound_red_butt_press, gsound_red_butt_release);
            }

            // Right offered inventory down button.
            btn = buttonCreate(i_wid, 333, 136, 22, 23, -1, -1, KEY_CTRL_PAGE_DOWN, -1, buttonUpData, buttonDownData, NULL, 0);
            if (btn != -1) {
                buttonSetCallbacks(btn, gsound_red_butt_press, gsound_red_butt_release);
            }
        }
    }

    i_rhand = NULL;
    i_worn = NULL;
    i_lhand = NULL;

    for (int index = 0; index < pud->length; index++) {
        InventoryItem* inventoryItem = &(pud->items[index]);
        Object* item = inventoryItem->item;
        if ((item->flags & OBJECT_IN_LEFT_HAND) != 0) {
            if ((item->flags & OBJECT_IN_RIGHT_HAND) != 0) {
                i_rhand = item;
            }
            i_lhand = item;
        } else if ((item->flags & OBJECT_IN_RIGHT_HAND) != 0) {
            i_rhand = item;
        } else if ((item->flags & OBJECT_WORN) != 0) {
            i_worn = item;
        }
    }

    if (i_lhand != NULL) {
        itemRemove(inven_dude, i_lhand, 1);
    }

    if (i_rhand != NULL && i_rhand != i_lhand) {
        itemRemove(inven_dude, i_rhand, 1);
    }

    if (i_worn != NULL) {
        itemRemove(inven_dude, i_worn, 1);
    }

    adjust_fid();

    bool isoWasEnabled = isoDisable();

    gmouse_disable(0);

    return isoWasEnabled;
}

// 0x46FBD8
void exit_inventory(bool shouldEnableIso)
{
    inven_dude = stack[0];

    if (i_lhand != NULL) {
        i_lhand->flags |= OBJECT_IN_LEFT_HAND;
        if (i_lhand == i_rhand) {
            i_lhand->flags |= OBJECT_IN_RIGHT_HAND;
        }

        itemAdd(inven_dude, i_lhand, 1);
    }

    if (i_rhand != NULL && i_rhand != i_lhand) {
        i_rhand->flags |= OBJECT_IN_RIGHT_HAND;
        itemAdd(inven_dude, i_rhand, 1);
    }

    if (i_worn != NULL) {
        i_worn->flags |= OBJECT_WORN;
        itemAdd(inven_dude, i_worn, 1);
    }

    i_rhand = NULL;
    i_worn = NULL;
    i_lhand = NULL;

    for (int index = 0; index < OFF_59E7BC_COUNT; index++) {
        art_ptr_unlock(ikey[index]);
    }

    if (shouldEnableIso) {
        isoEnable();
    }

    windowDestroy(i_wid);

    gmouse_enable();

    if (dropped_explosive) {
        Attack v1;
        combat_ctd_init(&v1, gDude, NULL, HIT_MODE_PUNCH, HIT_LOCATION_TORSO);
        v1.attackerFlags = DAM_HIT;
        v1.tile = gDude->tile;
        compute_explosion_on_extras(&v1, 0, 0, 1);

        Object* v2 = NULL;
        for (int index = 0; index < v1.extrasLength; index++) {
            Object* critter = v1.extras[index];
            if (critter != gDude
                && critter->data.critter.combat.team != gDude->data.critter.combat.team
                && statRoll(critter, STAT_PERCEPTION, 0, NULL) >= ROLL_SUCCESS) {
                critter_set_who_hit_me(critter, gDude);

                if (v2 == NULL) {
                    v2 = critter;
                }
            }
        }

        if (v2 != NULL) {
            if (!isInCombat()) {
                STRUCT_664980 v3;
                v3.attacker = v2;
                v3.defender = gDude;
                v3.actionPointsBonus = 0;
                v3.accuracyBonus = 0;
                v3.damageBonus = 0;
                v3.minDamage = 0;
                v3.maxDamage = INT_MAX;
                v3.field_1C = 0;
                scriptsRequestCombat(&v3);
            }
        }

        dropped_explosive = false;
    }
}

// 0x46FDF4
void display_inventory(int a1, int a2, int inventoryWindowType)
{
    unsigned char* windowBuffer = windowGetBuffer(i_wid);
    int pitch;

    int v49 = 0;
    if (inventoryWindowType == INVENTORY_WINDOW_TYPE_NORMAL) {
        pitch = 499;

        int backgroundFid = art_id(OBJ_TYPE_INTERFACE, 48, 0, 0, 0);

        CacheEntry* backgroundFrmHandle;
        unsigned char* backgroundFrmData = art_ptr_lock_data(backgroundFid, 0, 0, &backgroundFrmHandle);
        if (backgroundFrmData != NULL) {
            // Clear scroll view background.
            blitBufferToBuffer(backgroundFrmData + pitch * 35 + 44, INVENTORY_SLOT_WIDTH, inven_cur_disp * INVENTORY_SLOT_HEIGHT, pitch, windowBuffer + pitch * 35 + 44, pitch);

            // Clear armor button background.
            blitBufferToBuffer(backgroundFrmData + pitch * INVENTORY_ARMOR_SLOT_Y + INVENTORY_ARMOR_SLOT_X, INVENTORY_LARGE_SLOT_WIDTH, INVENTORY_LARGE_SLOT_HEIGHT, pitch, windowBuffer + pitch * INVENTORY_ARMOR_SLOT_Y + INVENTORY_ARMOR_SLOT_X, pitch);

            if (i_lhand != NULL && i_lhand == i_rhand) {
                // Clear item1.
                int itemBackgroundFid = art_id(OBJ_TYPE_INTERFACE, 32, 0, 0, 0);

                CacheEntry* itemBackgroundFrmHandle;
                Art* itemBackgroundFrm = art_ptr_lock(itemBackgroundFid, &itemBackgroundFrmHandle);
                if (itemBackgroundFrm != NULL) {
                    unsigned char* data = art_frame_data(itemBackgroundFrm, 0, 0);
                    int width = art_frame_width(itemBackgroundFrm, 0, 0);
                    int height = art_frame_length(itemBackgroundFrm, 0, 0);
                    blitBufferToBuffer(data, width, height, width, windowBuffer + pitch * 284 + 152, pitch);
                    art_ptr_unlock(itemBackgroundFrmHandle);
                }
            } else {
                // Clear both items in one go.
                blitBufferToBuffer(backgroundFrmData + pitch * INVENTORY_LEFT_HAND_SLOT_Y + INVENTORY_LEFT_HAND_SLOT_X, INVENTORY_LARGE_SLOT_WIDTH * 2, INVENTORY_LARGE_SLOT_HEIGHT, pitch, windowBuffer + pitch * INVENTORY_LEFT_HAND_SLOT_Y + INVENTORY_LEFT_HAND_SLOT_X, pitch);
            }

            art_ptr_unlock(backgroundFrmHandle);
        }
    } else if (inventoryWindowType == INVENTORY_WINDOW_TYPE_USE_ITEM_ON) {
        pitch = 292;

        int backgroundFid = art_id(OBJ_TYPE_INTERFACE, 113, 0, 0, 0);

        CacheEntry* backgroundFrmHandle;
        unsigned char* backgroundFrmData = art_ptr_lock_data(backgroundFid, 0, 0, &backgroundFrmHandle);
        if (backgroundFrmData != NULL) {
            // Clear scroll view background.
            blitBufferToBuffer(backgroundFrmData + pitch * 35 + 44, 64, inven_cur_disp * 48, pitch, windowBuffer + pitch * 35 + 44, pitch);
            art_ptr_unlock(backgroundFrmHandle);
        }
    } else if (inventoryWindowType == INVENTORY_WINDOW_TYPE_LOOT) {
        pitch = 537;

        int backgroundFid = art_id(OBJ_TYPE_INTERFACE, 114, 0, 0, 0);

        CacheEntry* backgroundFrmHandle;
        unsigned char* backgroundFrmData = art_ptr_lock_data(backgroundFid, 0, 0, &backgroundFrmHandle);
        if (backgroundFrmData != NULL) {
            // Clear scroll view background.
            blitBufferToBuffer(backgroundFrmData + pitch * 37 + 176, 64, inven_cur_disp * 48, pitch, windowBuffer + pitch * 37 + 176, pitch);
            art_ptr_unlock(backgroundFrmHandle);
        }
    } else if (inventoryWindowType == INVENTORY_WINDOW_TYPE_TRADE) {
        pitch = 480;

        windowBuffer = windowGetBuffer(i_wid);

        blitBufferToBuffer(windowGetBuffer(barter_back_win) + 35 * (_scr_size.right - _scr_size.left + 1) + 100, 64, 48 * inven_cur_disp, _scr_size.right - _scr_size.left + 1, windowBuffer + pitch * 35 + 20, pitch);
        v49 = -20;
    } else {
        assert(false && "Should be unreachable");
    }

    if (inventoryWindowType == INVENTORY_WINDOW_TYPE_NORMAL
        || inventoryWindowType == INVENTORY_WINDOW_TYPE_USE_ITEM_ON
        || inventoryWindowType == INVENTORY_WINDOW_TYPE_LOOT) {
        if (inven_scroll_up_bid != -1) {
            if (a1 <= 0) {
                buttonDisable(inven_scroll_up_bid);
            } else {
                buttonEnable(inven_scroll_up_bid);
            }
        }

        if (inven_scroll_dn_bid != -1) {
            if (pud->length - a1 <= inven_cur_disp) {
                buttonDisable(inven_scroll_dn_bid);
            } else {
                buttonEnable(inven_scroll_dn_bid);
            }
        }
    }

    int y = 0;
    for (int v19 = 0; v19 + a1 < pud->length && v19 < inven_cur_disp; v19 += 1) {
        int v21 = v19 + a1 + 1;

        int width;
        int offset;
        if (inventoryWindowType == INVENTORY_WINDOW_TYPE_TRADE) {
            offset = pitch * (y + 39) + 26;
            width = 59;
        } else {
            if (inventoryWindowType == INVENTORY_WINDOW_TYPE_LOOT) {
                offset = pitch * (y + 41) + 180;
            } else {
                offset = pitch * (y + 39) + 48;
            }
            width = 56;
        }

        InventoryItem* inventoryItem = &(pud->items[pud->length - v21]);

        int inventoryFid = itemGetInventoryFid(inventoryItem->item);
        scale_art(inventoryFid, windowBuffer + offset, width, 40, pitch);

        if (inventoryWindowType == INVENTORY_WINDOW_TYPE_LOOT) {
            offset = pitch * (y + 41) + 180 + v49;
        } else {
            offset = pitch * (y + 39) + 48 + v49;
        }

        display_inventory_info(inventoryItem->item, inventoryItem->quantity, windowBuffer + offset, pitch, v19 == a2);

        y += 48;
    }

    if (inventoryWindowType == INVENTORY_WINDOW_TYPE_NORMAL) {
        if (i_rhand != NULL) {
            int width = i_rhand == i_lhand ? INVENTORY_LARGE_SLOT_WIDTH * 2 : INVENTORY_LARGE_SLOT_WIDTH;
            int inventoryFid = itemGetInventoryFid(i_rhand);
            scale_art(inventoryFid, windowBuffer + 499 * INVENTORY_RIGHT_HAND_SLOT_Y + INVENTORY_RIGHT_HAND_SLOT_X, width, INVENTORY_LARGE_SLOT_HEIGHT, 499);
        }

        if (i_lhand != NULL && i_lhand != i_rhand) {
            int inventoryFid = itemGetInventoryFid(i_lhand);
            scale_art(inventoryFid, windowBuffer + 499 * INVENTORY_LEFT_HAND_SLOT_Y + INVENTORY_LEFT_HAND_SLOT_X, INVENTORY_LARGE_SLOT_WIDTH, INVENTORY_LARGE_SLOT_HEIGHT, 499);
        }

        if (i_worn != NULL) {
            int inventoryFid = itemGetInventoryFid(i_worn);
            scale_art(inventoryFid, windowBuffer + 499 * INVENTORY_ARMOR_SLOT_Y + INVENTORY_ARMOR_SLOT_X, INVENTORY_LARGE_SLOT_WIDTH, INVENTORY_LARGE_SLOT_HEIGHT, 499);
        }
    }

    win_draw(i_wid);
}

// Render inventory item.
//
// [a1] is likely an index of the first visible item in the scrolling view.
// [a2] is likely an index of selected item or moving item (it decreases displayed number of items in inner functions).
//
// 0x47036C
void display_target_inventory(int a1, int a2, Inventory* inventory, int inventoryWindowType)
{
    unsigned char* windowBuffer = windowGetBuffer(i_wid);

    int pitch;
    if (inventoryWindowType == INVENTORY_WINDOW_TYPE_LOOT) {
        pitch = 537;

        int fid = art_id(OBJ_TYPE_INTERFACE, 114, 0, 0, 0);

        CacheEntry* handle;
        unsigned char* data = art_ptr_lock_data(fid, 0, 0, &handle);
        if (data != NULL) {
            blitBufferToBuffer(data + 537 * 37 + 297, 64, 48 * inven_cur_disp, 537, windowBuffer + 537 * 37 + 297, 537);
            art_ptr_unlock(handle);
        }
    } else if (inventoryWindowType == INVENTORY_WINDOW_TYPE_TRADE) {
        pitch = 480;

        unsigned char* src = windowGetBuffer(barter_back_win);
        blitBufferToBuffer(src + (_scr_size.right - _scr_size.left + 1) * 35 + 475, 64, 48 * inven_cur_disp, _scr_size.right - _scr_size.left + 1, windowBuffer + 480 * 35 + 395, 480);
    } else {
        assert(false && "Should be unreachable");
    }

    int y = 0;
    for (int index = 0; index < inven_cur_disp; index++) {
        int v27 = a1 + index;
        if (v27 >= inventory->length) {
            break;
        }

        int offset;
        if (inventoryWindowType == INVENTORY_WINDOW_TYPE_LOOT) {
            offset = pitch * (y + 41) + 301;
        } else if (inventoryWindowType == INVENTORY_WINDOW_TYPE_TRADE) {
            offset = pitch * (y + 39) + 397;
        } else {
            assert(false && "Should be unreachable");
        }

        InventoryItem* inventoryItem = &(inventory->items[inventory->length - (v27 + 1)]);
        int inventoryFid = itemGetInventoryFid(inventoryItem->item);
        scale_art(inventoryFid, windowBuffer + offset, 56, 40, pitch);
        display_inventory_info(inventoryItem->item, inventoryItem->quantity, windowBuffer + offset, pitch, index == a2);

        y += 48;
    }

    if (inventoryWindowType == INVENTORY_WINDOW_TYPE_LOOT) {
        if (loot_scroll_up_bid != -1) {
            if (a1 <= 0) {
                buttonDisable(loot_scroll_up_bid);
            } else {
                buttonEnable(loot_scroll_up_bid);
            }
        }

        if (loot_scroll_dn_bid != -1) {
            if (inventory->length - a1 <= inven_cur_disp) {
                buttonDisable(loot_scroll_dn_bid);
            } else {
                buttonEnable(loot_scroll_dn_bid);
            }
        }
    }
}

// Renders inventory item quantity.
//
// 0x4705A0
static void display_inventory_info(Object* item, int quantity, unsigned char* dest, int pitch, bool a5)
{
    int oldFont = fontGetCurrent();
    fontSetCurrent(101);

    char formattedText[12];

    // NOTE: Original code is slightly different and probably used goto.
    bool draw = false;

    if (itemGetType(item) == ITEM_TYPE_AMMO) {
        int ammoQuantity = ammoGetCapacity(item) * (quantity - 1);

        if (!a5) {
            ammoQuantity += ammoGetQuantity(item);
        }

        if (ammoQuantity > 99999) {
            ammoQuantity = 99999;
        }

        sprintf(formattedText, "x%d", ammoQuantity);
        draw = true;
    } else {
        if (quantity > 1) {
            int v9 = quantity;
            if (a5) {
                v9 -= 1;
            }

            // NOTE: Checking for quantity twice probably means inlined function
            // or some macro expansion.
            if (quantity > 1) {
                if (v9 > 99999) {
                    v9 = 99999;
                }

                sprintf(formattedText, "x%d", v9);
                draw = true;
            }
        }
    }

    if (draw) {
        fontDrawText(dest, formattedText, 80, pitch, colorTable[32767]);
    }

    fontSetCurrent(oldFont);
}

// 0x470650
void display_body(int fid, int inventoryWindowType)
{
    // 0x5190F4
    static unsigned int ticker = 0;

    // 0x5190F8
    static int curr_rot = 0;

    if (getTicksSince(ticker) < INVENTORY_NORMAL_WINDOW_PC_ROTATION_DELAY) {
        return;
    }

    curr_rot += 1;

    if (curr_rot == ROTATION_COUNT) {
        curr_rot = 0;
    }

    int rotations[2];
    if (fid == -1) {
        rotations[0] = curr_rot;
        rotations[1] = ROTATION_SE;
    } else {
        rotations[0] = ROTATION_SW;
        rotations[1] = target_stack[target_curr_stack]->rotation;
    }

    int fids[2] = {
        i_fid,
        fid,
    };

    for (int index = 0; index < 2; index += 1) {
        int fid = fids[index];
        if (fid == -1) {
            continue;
        }

        CacheEntry* handle;
        Art* art = art_ptr_lock(fid, &handle);
        if (art == NULL) {
            continue;
        }

        int frame = 0;
        if (index == 1) {
            frame = art_frame_max_frame(art) - 1;
        }

        int rotation = rotations[index];

        unsigned char* frameData = art_frame_data(art, frame, rotation);

        int framePitch = art_frame_width(art, frame, rotation);
        int frameWidth = min(framePitch, INVENTORY_BODY_VIEW_WIDTH);

        int frameHeight = art_frame_length(art, frame, rotation);
        if (frameHeight > INVENTORY_BODY_VIEW_HEIGHT) {
            frameHeight = INVENTORY_BODY_VIEW_HEIGHT;
        }

        int win;
        Rect rect;
        CacheEntry* backrgroundFrmHandle;
        if (inventoryWindowType == INVENTORY_WINDOW_TYPE_TRADE) {
            unsigned char* windowBuffer = windowGetBuffer(barter_back_win);
            int windowPitch = windowGetWidth(barter_back_win);

            if (index == 1) {
                rect.left = 560;
                rect.top = 25;
            } else {
                rect.left = 15;
                rect.top = 25;
            }

            rect.right = rect.left + INVENTORY_BODY_VIEW_WIDTH - 1;
            rect.bottom = rect.top + INVENTORY_BODY_VIEW_HEIGHT - 1;

            int frmId = dialog_target_is_party ? 420 : 111;
            int backgroundFid = art_id(OBJ_TYPE_INTERFACE, frmId, 0, 0, 0);

            unsigned char* src = art_ptr_lock_data(backgroundFid, 0, 0, &backrgroundFrmHandle);
            if (src != NULL) {
                blitBufferToBuffer(src + rect.top * (_scr_size.right - _scr_size.left + 1) + rect.left,
                    INVENTORY_BODY_VIEW_WIDTH,
                    INVENTORY_BODY_VIEW_HEIGHT,
                    _scr_size.right - _scr_size.left + 1,
                    windowBuffer + windowPitch * rect.top + rect.left,
                    windowPitch);
            }

            blitBufferToBufferTrans(frameData, frameWidth, frameHeight, framePitch,
                windowBuffer + windowPitch * (rect.top + (INVENTORY_BODY_VIEW_HEIGHT - frameHeight) / 2) + (INVENTORY_BODY_VIEW_WIDTH - frameWidth) / 2 + rect.left,
                windowPitch);

            win = barter_back_win;
        } else {
            unsigned char* windowBuffer = windowGetBuffer(i_wid);
            int windowPitch = windowGetWidth(i_wid);

            if (index == 1) {
                if (inventoryWindowType == INVENTORY_WINDOW_TYPE_LOOT) {
                    rect.left = 426;
                    rect.top = 39;
                } else {
                    rect.left = 297;
                    rect.top = 37;
                }
            } else {
                if (inventoryWindowType == INVENTORY_WINDOW_TYPE_LOOT) {
                    rect.left = 48;
                    rect.top = 39;
                } else {
                    rect.left = 176;
                    rect.top = 37;
                }
            }

            rect.right = rect.left + INVENTORY_BODY_VIEW_WIDTH - 1;
            rect.bottom = rect.top + INVENTORY_BODY_VIEW_HEIGHT - 1;

            int backgroundFid = art_id(OBJ_TYPE_INTERFACE, 114, 0, 0, 0);
            unsigned char* src = art_ptr_lock_data(backgroundFid, 0, 0, &backrgroundFrmHandle);
            if (src != NULL) {
                blitBufferToBuffer(src + 537 * rect.top + rect.left,
                    INVENTORY_BODY_VIEW_WIDTH,
                    INVENTORY_BODY_VIEW_HEIGHT,
                    537,
                    windowBuffer + windowPitch * rect.top + rect.left,
                    windowPitch);
            }

            blitBufferToBufferTrans(frameData, frameWidth, frameHeight, framePitch,
                windowBuffer + windowPitch * (rect.top + (INVENTORY_BODY_VIEW_HEIGHT - frameHeight) / 2) + (INVENTORY_BODY_VIEW_WIDTH - frameWidth) / 2 + rect.left,
                windowPitch);

            win = i_wid;
        }
        win_draw_rect(win, &rect);

        art_ptr_unlock(backrgroundFrmHandle);
        art_ptr_unlock(handle);
    }

    ticker = _get_time();
}

// 0x470A2C
int inven_init()
{
    // 0x5190FC
    static int num[INVENTORY_WINDOW_CURSOR_COUNT] = {
        286, // pointing hand
        250, // action arrow
        282, // action pick
        283, // action menu
        266, // blank
    };

    if (inventry_msg_load() == -1) {
        return -1;
    }

    inven_ui_was_disabled = game_ui_is_disabled();

    if (inven_ui_was_disabled) {
        game_ui_enable();
    }

    gmouse_3d_off();

    gmouse_set_cursor(MOUSE_CURSOR_ARROW);

    int index;
    for (index = 0; index < INVENTORY_WINDOW_CURSOR_COUNT; index++) {
        InventoryCursorData* cursorData = &(imdata[index]);

        int fid = art_id(OBJ_TYPE_INTERFACE, num[index], 0, 0, 0);
        Art* frm = art_ptr_lock(fid, &(cursorData->frmHandle));
        if (frm == NULL) {
            break;
        }

        cursorData->frm = frm;
        cursorData->frmData = art_frame_data(frm, 0, 0);
        cursorData->width = art_frame_width(frm, 0, 0);
        cursorData->height = art_frame_length(frm, 0, 0);
        art_frame_hot(frm, 0, 0, &(cursorData->offsetX), &(cursorData->offsetY));
    }

    if (index != INVENTORY_WINDOW_CURSOR_COUNT) {
        for (; index >= 0; index--) {
            art_ptr_unlock(imdata[index].frmHandle);
        }

        if (inven_ui_was_disabled) {
            game_ui_disable(0);
        }

        messageListFree(&inventry_message_file);

        return -1;
    }

    inven_is_initialized = true;
    im_value = -1;

    return 0;
}

// NOTE: Inlined.
//
// 0x470B8C
void inven_exit()
{
    for (int index = 0; index < INVENTORY_WINDOW_CURSOR_COUNT; index++) {
        art_ptr_unlock(imdata[index].frmHandle);
    }

    if (inven_ui_was_disabled) {
        game_ui_disable(0);
    }

    // NOTE: Uninline.
    inventry_msg_unload();

    inven_is_initialized = 0;
}

// 0x470BCC
void inven_set_mouse(int cursor)
{
    immode = cursor;

    if (cursor != INVENTORY_WINDOW_CURSOR_ARROW || im_value == -1) {
        InventoryCursorData* cursorData = &(imdata[cursor]);
        mouse_set_shape(cursorData->frmData, cursorData->width, cursorData->height, cursorData->width, cursorData->offsetX, cursorData->offsetY, 0);
    } else {
        inven_hover_on(-1, im_value);
    }
}

// 0x470C2C
void inven_hover_on(int btn, int keyCode)
{
    // 0x519110
    static Object* last_target = NULL;

    if (immode == INVENTORY_WINDOW_CURSOR_ARROW) {
        int x;
        int y;
        mouse_get_position(&x, &y);

        Object* a2a = NULL;
        if (inven_from_button(keyCode, &a2a, NULL, NULL) != 0) {
            gmouse_3d_build_pick_frame(x, y, 3, i_wid_max_x, i_wid_max_y);

            int v5 = 0;
            int v6 = 0;
            gmouse_3d_pick_frame_hot(&v5, &v6);

            InventoryCursorData* cursorData = &(imdata[INVENTORY_WINDOW_CURSOR_PICK]);
            mouse_set_shape(cursorData->frmData, cursorData->width, cursorData->height, cursorData->width, v5, v6, 0);

            if (a2a != last_target) {
                _obj_look_at_func(stack[0], a2a, display_msg);
            }
        } else {
            InventoryCursorData* cursorData = &(imdata[INVENTORY_WINDOW_CURSOR_ARROW]);
            mouse_set_shape(cursorData->frmData, cursorData->width, cursorData->height, cursorData->width, cursorData->offsetX, cursorData->offsetY, 0);
        }

        last_target = a2a;
    }

    im_value = keyCode;
}

// 0x470D1C
void inven_hover_off(int btn, int keyCode)
{
    if (immode == INVENTORY_WINDOW_CURSOR_ARROW) {
        InventoryCursorData* cursorData = &(imdata[INVENTORY_WINDOW_CURSOR_ARROW]);
        mouse_set_shape(cursorData->frmData, cursorData->width, cursorData->height, cursorData->width, cursorData->offsetX, cursorData->offsetY, 0);
    }

    im_value = -1;
}

// 0x470D5C
static void inven_update_lighting(Object* a1)
{
    if (gDude == inven_dude) {
        int lightDistance;
        if (a1 != NULL && a1->lightDistance > 4) {
            lightDistance = a1->lightDistance;
        } else {
            lightDistance = 4;
        }

        Rect rect;
        objectSetLight(inven_dude, lightDistance, 0x10000, &rect);
        tileWindowRefreshRect(&rect, gElevation);
    }
}

// 0x470DB8
void inven_pickup(int keyCode, int a2)
{
    Object* a1a;
    Object** v29 = NULL;
    int count = inven_from_button(keyCode, &a1a, &v29, NULL);
    if (count == 0) {
        return;
    }

    int v3 = -1;
    Object* v39 = NULL;
    Rect rect;

    switch (keyCode) {
    case 1006:
        rect.left = 245;
        rect.top = 286;
        if (inven_dude == gDude && intface_is_item_right_hand() != HAND_LEFT) {
            v39 = a1a;
        }
        break;
    case 1007:
        rect.left = 154;
        rect.top = 286;
        if (inven_dude == gDude && intface_is_item_right_hand() == HAND_LEFT) {
            v39 = a1a;
        }
        break;
    case 1008:
        rect.left = 154;
        rect.top = 183;
        break;
    default:
        // NOTE: Original code a little bit different, this code path
        // is only for key codes below 1006.
        v3 = keyCode - 1000;
        rect.left = 44;
        rect.top = 48 * v3 + 35;
        break;
    }

    if (v3 == -1 || pud->items[a2 + v3].quantity <= 1) {
        unsigned char* windowBuffer = windowGetBuffer(i_wid);
        if (i_rhand != i_lhand || a1a != i_lhand) {
            int height;
            int width;
            if (v3 == -1) {
                height = INVENTORY_LARGE_SLOT_HEIGHT;
                width = INVENTORY_LARGE_SLOT_WIDTH;
            } else {
                height = INVENTORY_SLOT_HEIGHT;
                width = INVENTORY_SLOT_WIDTH;
            }

            CacheEntry* backgroundFrmHandle;
            int backgroundFid = art_id(OBJ_TYPE_INTERFACE, 48, 0, 0, 0);
            unsigned char* backgroundFrmData = art_ptr_lock_data(backgroundFid, 0, 0, &backgroundFrmHandle);
            if (backgroundFrmData != NULL) {
                blitBufferToBuffer(backgroundFrmData + 499 * rect.top + rect.left, width, height, 499, windowBuffer + 499 * rect.top + rect.left, 499);
                art_ptr_unlock(backgroundFrmHandle);
            }

            rect.right = rect.left + width - 1;
            rect.bottom = rect.top + height - 1;
        } else {
            CacheEntry* backgroundFrmHandle;
            int backgroundFid = art_id(OBJ_TYPE_INTERFACE, 48, 0, 0, 0);
            unsigned char* backgroundFrmData = art_ptr_lock_data(backgroundFid, 0, 0, &backgroundFrmHandle);
            if (backgroundFrmData != NULL) {
                blitBufferToBuffer(backgroundFrmData + 499 * 286 + 154, 180, 61, 499, windowBuffer + 499 * 286 + 154, 499);
                art_ptr_unlock(backgroundFrmHandle);
            }

            rect.left = 154;
            rect.top = 286;
            rect.right = rect.left + 180 - 1;
            rect.bottom = rect.top + 61 - 1;
        }
        win_draw_rect(i_wid, &rect);
    } else {
        display_inventory(a2, v3, INVENTORY_WINDOW_TYPE_NORMAL);
    }

    CacheEntry* itemInventoryFrmHandle;
    int itemInventoryFid = itemGetInventoryFid(a1a);
    Art* itemInventoryFrm = art_ptr_lock(itemInventoryFid, &itemInventoryFrmHandle);
    if (itemInventoryFrm != NULL) {
        int width = art_frame_width(itemInventoryFrm, 0, 0);
        int height = art_frame_length(itemInventoryFrm, 0, 0);
        unsigned char* itemInventoryFrmData = art_frame_data(itemInventoryFrm, 0, 0);
        mouse_set_shape(itemInventoryFrmData, width, height, width, width / 2, height / 2, 0);
        gsound_play_sfx_file("ipickup1");
    }

    if (v39 != NULL) {
        inven_update_lighting(NULL);
    }

    do {
        _get_input();
        display_body(-1, INVENTORY_WINDOW_TYPE_NORMAL);
    } while ((mouse_get_buttons() & MOUSE_EVENT_LEFT_BUTTON_REPEAT) != 0);

    if (itemInventoryFrm != NULL) {
        art_ptr_unlock(itemInventoryFrmHandle);
        gsound_play_sfx_file("iputdown");
    }

    if (mouse_click_in(INVENTORY_SCROLLER_ABS_X, INVENTORY_SCROLLER_ABS_Y, INVENTORY_SCROLLER_ABS_MAX_X, INVENTORY_SLOT_HEIGHT * inven_cur_disp + INVENTORY_SCROLLER_ABS_Y)) {
        int x;
        int y;
        mouse_get_position(&x, &y);

        int v18 = (y - 39) / 48 + a2;
        if (v18 < pud->length) {
            Object* v19 = pud->items[v18].item;
            if (v19 != a1a) {
                // TODO: Needs checking usage of v19
                if (itemGetType(v19) == ITEM_TYPE_CONTAINER) {
                    if (drop_into_container(v19, a1a, v3, v29, count) == 0) {
                        v3 = 0;
                    }
                } else {
                    if (drop_ammo_into_weapon(v19, a1a, v29, count, keyCode) == 0) {
                        v3 = 0;
                    }
                }
            }
        }

        if (v3 == -1) {
            // TODO: Holy shit, needs refactoring.
            *v29 = NULL;
            if (itemAdd(inven_dude, a1a, 1)) {
                *v29 = a1a;
            } else if (v29 == &i_worn) {
                adjust_ac(stack[0], a1a, NULL);
            } else if (i_rhand == i_lhand) {
                i_lhand = NULL;
                i_rhand = NULL;
            }
        }
    } else if (mouse_click_in(INVENTORY_LEFT_HAND_SLOT_ABS_X, INVENTORY_LEFT_HAND_SLOT_ABS_Y, INVENTORY_LEFT_HAND_SLOT_ABS_MAX_X, INVENTORY_LEFT_HAND_SLOT_ABS_MAX_Y)) {
        if (i_lhand != NULL && itemGetType(i_lhand) == ITEM_TYPE_CONTAINER && i_lhand != a1a) {
            drop_into_container(i_lhand, a1a, v3, v29, count);
        } else if (i_lhand == NULL || drop_ammo_into_weapon(i_lhand, a1a, v29, count, keyCode)) {
            switch_hand(a1a, &i_lhand, v29, keyCode);
        }
    } else if (mouse_click_in(INVENTORY_RIGHT_HAND_SLOT_ABS_X, INVENTORY_RIGHT_HAND_SLOT_ABS_Y, INVENTORY_RIGHT_HAND_SLOT_ABS_MAX_X, INVENTORY_RIGHT_HAND_SLOT_ABS_MAX_Y)) {
        if (i_rhand != NULL && itemGetType(i_rhand) == ITEM_TYPE_CONTAINER && i_rhand != a1a) {
            drop_into_container(i_rhand, a1a, v3, v29, count);
        } else if (i_rhand == NULL || drop_ammo_into_weapon(i_rhand, a1a, v29, count, keyCode)) {
            switch_hand(a1a, &i_rhand, v29, v3);
        }
    } else if (mouse_click_in(INVENTORY_ARMOR_SLOT_ABS_X, INVENTORY_ARMOR_SLOT_ABS_Y, INVENTORY_ARMOR_SLOT_ABS_MAX_X, INVENTORY_ARMOR_SLOT_ABS_MAX_Y)) {
        if (itemGetType(a1a) == ITEM_TYPE_ARMOR) {
            Object* v21 = i_worn;
            int v22 = 0;
            if (v3 != -1) {
                itemRemove(inven_dude, a1a, 1);
            }

            if (i_worn != NULL) {
                if (v29 != NULL) {
                    *v29 = i_worn;
                } else {
                    i_worn = NULL;
                    v22 = itemAdd(inven_dude, v21, 1);
                }
            } else {
                if (v29 != NULL) {
                    *v29 = i_worn;
                }
            }

            if (v22 != 0) {
                i_worn = v21;
                if (v3 != -1) {
                    itemAdd(inven_dude, a1a, 1);
                }
            } else {
                adjust_ac(stack[0], v21, a1a);
                i_worn = a1a;
            }
        }
    } else if (mouse_click_in(INVENTORY_PC_BODY_VIEW_ABS_X, INVENTORY_PC_BODY_VIEW_ABS_Y, INVENTORY_PC_BODY_VIEW_ABS_MAX_X, INVENTORY_PC_BODY_VIEW_ABS_MAX_Y)) {
        if (curr_stack != 0) {
            // TODO: Check this curr_stack - 1, not sure.
            drop_into_container(stack[curr_stack - 1], a1a, v3, v29, count);
        }
    }

    adjust_fid();
    display_stats();
    display_inventory(a2, -1, INVENTORY_WINDOW_TYPE_NORMAL);
    inven_set_mouse(INVENTORY_WINDOW_CURSOR_HAND);
    if (inven_dude == gDude) {
        Object* item;
        if (intface_is_item_right_hand() == HAND_LEFT) {
            item = inven_left_hand(inven_dude);
        } else {
            item = inven_right_hand(inven_dude);
        }

        if (item != NULL) {
            inven_update_lighting(item);
        }
    }
}

// 0x4714E0
void switch_hand(Object* a1, Object** a2, Object** a3, int a4)
{
    if (*a2 != NULL) {
        if (itemGetType(*a2) == ITEM_TYPE_WEAPON && itemGetType(a1) == ITEM_TYPE_AMMO) {
            return;
        }

        if (a3 != NULL && (a3 != &i_worn || itemGetType(*a2) == ITEM_TYPE_ARMOR)) {
            if (a3 == &i_worn) {
                adjust_ac(stack[0], i_worn, *a2);
            }
            *a3 = *a2;
        } else {
            if (a4 != -1) {
                itemRemove(inven_dude, a1, 1);
            }

            Object* itemToAdd = *a2;
            *a2 = NULL;
            if (itemAdd(inven_dude, itemToAdd, 1) != 0) {
                itemAdd(inven_dude, a1, 1);
                return;
            }

            a4 = -1;

            if (a3 != NULL) {
                if (a3 == &i_worn) {
                    adjust_ac(stack[0], i_worn, NULL);
                }
                *a3 = NULL;
            }
        }
    } else {
        if (a3 != NULL) {
            if (a3 == &i_worn) {
                adjust_ac(stack[0], i_worn, NULL);
            }
            *a3 = NULL;
        }
    }

    *a2 = a1;

    if (a4 != -1) {
        itemRemove(inven_dude, a1, 1);
    }
}

// This function removes armor bonuses and effects granted by [oldArmor] and
// adds appropriate bonuses and effects granted by [newArmor]. Both [oldArmor]
// and [newArmor] can be NULL.
//
// 0x4715F8
void adjust_ac(Object* critter, Object* oldArmor, Object* newArmor)
{
    int armorClassBonus = critterGetBonusStat(critter, STAT_ARMOR_CLASS);
    int oldArmorClass = armorGetArmorClass(oldArmor);
    int newArmorClass = armorGetArmorClass(newArmor);
    critterSetBonusStat(critter, STAT_ARMOR_CLASS, armorClassBonus - oldArmorClass + newArmorClass);

    int damageResistanceStat = STAT_DAMAGE_RESISTANCE;
    int damageThresholdStat = STAT_DAMAGE_THRESHOLD;
    for (int damageType = 0; damageType < DAMAGE_TYPE_COUNT; damageType += 1) {
        int damageResistanceBonus = critterGetBonusStat(critter, damageResistanceStat);
        int oldArmorDamageResistance = armorGetDamageResistance(oldArmor, damageType);
        int newArmorDamageResistance = armorGetDamageResistance(newArmor, damageType);
        critterSetBonusStat(critter, damageResistanceStat, damageResistanceBonus - oldArmorDamageResistance + newArmorDamageResistance);

        int damageThresholdBonus = critterGetBonusStat(critter, damageThresholdStat);
        int oldArmorDamageThreshold = armorGetDamageThreshold(oldArmor, damageType);
        int newArmorDamageThreshold = armorGetDamageThreshold(newArmor, damageType);
        critterSetBonusStat(critter, damageThresholdStat, damageThresholdBonus - oldArmorDamageThreshold + newArmorDamageThreshold);

        damageResistanceStat += 1;
        damageThresholdStat += 1;
    }

    if (objectIsPartyMember(critter)) {
        if (oldArmor != NULL) {
            int perk = armorGetPerk(oldArmor);
            perkRemoveEffect(critter, perk);
        }

        if (newArmor != NULL) {
            int perk = armorGetPerk(newArmor);
            perkAddEffect(critter, perk);
        }
    }
}

// 0x4716E8
void adjust_fid()
{
    int fid;
    if (FID_TYPE(inven_dude->fid) == OBJ_TYPE_CRITTER) {
        Proto* proto;

        int v0 = art_vault_guy_num;

        if (protoGetProto(inven_pid, &proto) == -1) {
            v0 = proto->fid & 0xFFF;
        }

        if (i_worn != NULL) {
            protoGetProto(i_worn->pid, &proto);
            if (critterGetStat(inven_dude, STAT_GENDER) == GENDER_FEMALE) {
                v0 = proto->item.data.armor.femaleFid;
            } else {
                v0 = proto->item.data.armor.maleFid;
            }

            if (v0 == -1) {
                v0 = art_vault_guy_num;
            }
        }

        int animationCode = 0;
        if (intface_is_item_right_hand()) {
            if (i_rhand != NULL) {
                protoGetProto(i_rhand->pid, &proto);
                if (proto->item.type == ITEM_TYPE_WEAPON) {
                    animationCode = proto->item.data.weapon.animationCode;
                }
            }
        } else {
            if (i_lhand != NULL) {
                protoGetProto(i_lhand->pid, &proto);
                if (proto->item.type == ITEM_TYPE_WEAPON) {
                    animationCode = proto->item.data.weapon.animationCode;
                }
            }
        }

        fid = art_id(OBJ_TYPE_CRITTER, v0, 0, animationCode, 0);
    } else {
        fid = inven_dude->fid;
    }

    i_fid = fid;
}

// 0x4717E4
void use_inventory_on(Object* a1)
{
    if (inven_init() == -1) {
        return;
    }

    bool isoWasEnabled = setup_inventory(INVENTORY_WINDOW_TYPE_USE_ITEM_ON);
    display_inventory(stack_offset[curr_stack], -1, INVENTORY_WINDOW_TYPE_USE_ITEM_ON);
    inven_set_mouse(INVENTORY_WINDOW_CURSOR_HAND);
    for (;;) {
        if (game_user_wants_to_quit != 0) {
            break;
        }

        display_body(-1, INVENTORY_WINDOW_TYPE_USE_ITEM_ON);

        int keyCode = _get_input();
        switch (keyCode) {
        case KEY_HOME:
            stack_offset[curr_stack] = 0;
            display_inventory(0, -1, INVENTORY_WINDOW_TYPE_USE_ITEM_ON);
            break;
        case KEY_ARROW_UP:
            if (stack_offset[curr_stack] > 0) {
                stack_offset[curr_stack] -= 1;
                display_inventory(stack_offset[curr_stack], -1, INVENTORY_WINDOW_TYPE_USE_ITEM_ON);
            }
            break;
        case KEY_PAGE_UP:
            stack_offset[curr_stack] -= inven_cur_disp;
            if (stack_offset[curr_stack] < 0) {
                stack_offset[curr_stack] = 0;
                display_inventory(stack_offset[curr_stack], -1, 1);
            }
            break;
        case KEY_END:
            stack_offset[curr_stack] = pud->length - inven_cur_disp;
            if (stack_offset[curr_stack] < 0) {
                stack_offset[curr_stack] = 0;
            }
            display_inventory(stack_offset[curr_stack], -1, INVENTORY_WINDOW_TYPE_USE_ITEM_ON);
            break;
        case KEY_ARROW_DOWN:
            if (stack_offset[curr_stack] + inven_cur_disp < pud->length) {
                stack_offset[curr_stack] += 1;
                display_inventory(stack_offset[curr_stack], -1, INVENTORY_WINDOW_TYPE_USE_ITEM_ON);
            }
            break;
        case KEY_PAGE_DOWN:
            stack_offset[curr_stack] += inven_cur_disp;
            if (stack_offset[curr_stack] + inven_cur_disp >= pud->length) {
                stack_offset[curr_stack] = pud->length - inven_cur_disp;
                if (stack_offset[curr_stack] < 0) {
                    stack_offset[curr_stack] = 0;
                }
            }
            display_inventory(stack_offset[curr_stack], -1, 1);
            break;
        case 2500:
            container_exit(keyCode, INVENTORY_WINDOW_TYPE_USE_ITEM_ON);
            break;
        default:
            if ((mouse_get_buttons() & MOUSE_EVENT_RIGHT_BUTTON_DOWN) != 0) {
                if (immode == INVENTORY_WINDOW_CURSOR_HAND) {
                    inven_set_mouse(INVENTORY_WINDOW_CURSOR_ARROW);
                } else {
                    inven_set_mouse(INVENTORY_WINDOW_CURSOR_HAND);
                }
            } else if ((mouse_get_buttons() & MOUSE_EVENT_LEFT_BUTTON_DOWN) != 0) {
                if (keyCode >= 1000 && keyCode < 1000 + inven_cur_disp) {
                    if (immode == INVENTORY_WINDOW_CURSOR_ARROW) {
                        inven_action_cursor(keyCode, INVENTORY_WINDOW_TYPE_USE_ITEM_ON);
                    } else {
                        int inventoryItemIndex = pud->length - (stack_offset[curr_stack] + keyCode - 1000 + 1);
                        if (inventoryItemIndex < pud->length) {
                            InventoryItem* inventoryItem = &(pud->items[inventoryItemIndex]);
                            if (isInCombat()) {
                                if (gDude->data.critter.combat.ap >= 2) {
                                    if (action_use_an_item_on_object(gDude, a1, inventoryItem->item) != -1) {
                                        int actionPoints = gDude->data.critter.combat.ap;
                                        if (actionPoints < 2) {
                                            gDude->data.critter.combat.ap = 0;
                                        } else {
                                            gDude->data.critter.combat.ap = actionPoints - 2;
                                        }
                                        intface_update_move_points(gDude->data.critter.combat.ap, combat_free_move);
                                    }
                                }
                            } else {
                                action_use_an_item_on_object(gDude, a1, inventoryItem->item);
                            }
                            keyCode = KEY_ESCAPE;
                        } else {
                            keyCode = -1;
                        }
                    }
                }
            }
        }

        if (keyCode == KEY_ESCAPE) {
            break;
        }
    }

    exit_inventory(isoWasEnabled);

    // NOTE: Uninline.
    inven_exit();
}

// 0x471B70
Object* inven_right_hand(Object* critter)
{
    int i;
    Inventory* inventory;
    Object* item;

    if (i_rhand != NULL && critter == inven_dude) {
        return i_rhand;
    }

    inventory = &(critter->data.inventory);
    for (i = 0; i < inventory->length; i++) {
        item = inventory->items[i].item;
        if (item->flags & OBJECT_IN_RIGHT_HAND) {
            return item;
        }
    }

    return NULL;
}

// 0x471BBC
Object* inven_left_hand(Object* critter)
{
    int i;
    Inventory* inventory;
    Object* item;

    if (i_lhand != NULL && critter == inven_dude) {
        return i_lhand;
    }

    inventory = &(critter->data.inventory);
    for (i = 0; i < inventory->length; i++) {
        item = inventory->items[i].item;
        if (item->flags & OBJECT_IN_LEFT_HAND) {
            return item;
        }
    }

    return NULL;
}

// 0x471C08
Object* inven_worn(Object* critter)
{
    int i;
    Inventory* inventory;
    Object* item;

    if (i_worn != NULL && critter == inven_dude) {
        return i_worn;
    }

    inventory = &(critter->data.inventory);
    for (i = 0; i < inventory->length; i++) {
        item = inventory->items[i].item;
        if (item->flags & OBJECT_WORN) {
            return item;
        }
    }

    return NULL;
}

// 0x471CA0
Object* inven_pid_is_carried(Object* obj, int pid)
{
    Inventory* inventory = &(obj->data.inventory);

    for (int index = 0; index < inventory->length; index++) {
        InventoryItem* inventoryItem = &(inventory->items[index]);
        if (inventoryItem->item->pid == pid) {
            return inventoryItem->item;
        }

        Object* found = inven_pid_is_carried(inventoryItem->item, pid);
        if (found != NULL) {
            return found;
        }
    }

    return NULL;
}

// 0x471CDC
int inven_pid_quantity_carried(Object* object, int pid)
{
    int quantity = 0;

    Inventory* inventory = &(object->data.inventory);
    for (int index = 0; index < inventory->length; index++) {
        InventoryItem* inventoryItem = &(inventory->items[index]);
        if (inventoryItem->item->pid == pid) {
            quantity += inventoryItem->quantity;
        }

        quantity += inven_pid_quantity_carried(inventoryItem->item, pid);
    }

    return quantity;
}

// Renders character's summary of SPECIAL stats, equipped armor bonuses,
// and weapon's damage/range.
//
// 0x471D5C
void display_stats()
{
    // 0x46E6D0
    static const int v56[7] = {
        STAT_CURRENT_HIT_POINTS,
        STAT_ARMOR_CLASS,
        STAT_DAMAGE_THRESHOLD,
        STAT_DAMAGE_THRESHOLD_LASER,
        STAT_DAMAGE_THRESHOLD_FIRE,
        STAT_DAMAGE_THRESHOLD_PLASMA,
        STAT_DAMAGE_THRESHOLD_EXPLOSION,
    };

    // 0x46E6EC
    static const int v57[7] = {
        STAT_MAXIMUM_HIT_POINTS,
        -1,
        STAT_DAMAGE_RESISTANCE,
        STAT_DAMAGE_RESISTANCE_LASER,
        STAT_DAMAGE_RESISTANCE_FIRE,
        STAT_DAMAGE_RESISTANCE_PLASMA,
        STAT_DAMAGE_RESISTANCE_EXPLOSION,
    };

    char formattedText[80];

    int oldFont = fontGetCurrent();
    fontSetCurrent(101);

    unsigned char* windowBuffer = windowGetBuffer(i_wid);

    int fid = art_id(OBJ_TYPE_INTERFACE, 48, 0, 0, 0);

    CacheEntry* backgroundHandle;
    unsigned char* backgroundData = art_ptr_lock_data(fid, 0, 0, &backgroundHandle);
    if (backgroundData != NULL) {
        blitBufferToBuffer(backgroundData + 499 * 44 + 297, 152, 188, 499, windowBuffer + 499 * 44 + 297, 499);
    }
    art_ptr_unlock(backgroundHandle);

    // Render character name.
    const char* critterName = critter_name(stack[0]);
    fontDrawText(windowBuffer + 499 * 44 + 297, critterName, 80, 499, colorTable[992]);

    bufferDrawLine(windowBuffer,
        499,
        297,
        3 * fontGetLineHeight() / 2 + 44,
        440,
        3 * fontGetLineHeight() / 2 + 44,
        colorTable[992]);

    MessageListItem messageListItem;

    int offset = 499 * 2 * fontGetLineHeight() + 499 * 44 + 297;
    for (int stat = 0; stat < 7; stat++) {
        messageListItem.num = stat;
        if (messageListGetItem(&inventry_message_file, &messageListItem)) {
            fontDrawText(windowBuffer + offset, messageListItem.text, 80, 499, colorTable[992]);
        }

        int value = critterGetStat(stack[0], stat);
        sprintf(formattedText, "%d", value);
        fontDrawText(windowBuffer + offset + 24, formattedText, 80, 499, colorTable[992]);

        offset += 499 * fontGetLineHeight();
    }

    offset -= 499 * 7 * fontGetLineHeight();

    for (int index = 0; index < 7; index += 1) {
        messageListItem.num = 7 + index;
        if (messageListGetItem(&inventry_message_file, &messageListItem)) {
            fontDrawText(windowBuffer + offset + 40, messageListItem.text, 80, 499, colorTable[992]);
        }

        if (v57[index] == -1) {
            int value = critterGetStat(stack[0], v56[index]);
            sprintf(formattedText, "   %d", value);
        } else {
            int value1 = critterGetStat(stack[0], v56[index]);
            int value2 = critterGetStat(stack[0], v57[index]);
            const char* format = index != 0 ? "%d/%d%%" : "%d/%d";
            sprintf(formattedText, format, value1, value2);
        }

        fontDrawText(windowBuffer + offset + 104, formattedText, 80, 499, colorTable[992]);

        offset += 499 * fontGetLineHeight();
    }

    bufferDrawLine(windowBuffer, 499, 297, 18 * fontGetLineHeight() / 2 + 48, 440, 18 * fontGetLineHeight() / 2 + 48, colorTable[992]);
    bufferDrawLine(windowBuffer, 499, 297, 26 * fontGetLineHeight() / 2 + 48, 440, 26 * fontGetLineHeight() / 2 + 48, colorTable[992]);

    Object* itemsInHands[2] = {
        i_lhand,
        i_rhand,
    };

    const int hitModes[2] = {
        HIT_MODE_LEFT_WEAPON_PRIMARY,
        HIT_MODE_RIGHT_WEAPON_PRIMARY,
    };

    offset += 499 * fontGetLineHeight();

    for (int index = 0; index < 2; index += 1) {
        Object* item = itemsInHands[index];
        if (item == NULL) {
            formattedText[0] = '\0';

            // No item
            messageListItem.num = 14;
            if (messageListGetItem(&inventry_message_file, &messageListItem)) {
                fontDrawText(windowBuffer + offset, messageListItem.text, 120, 499, colorTable[992]);
            }

            offset += 499 * fontGetLineHeight();

            // Unarmed dmg:
            messageListItem.num = 24;
            if (messageListGetItem(&inventry_message_file, &messageListItem)) {
                // TODO: Figure out why it uses STAT_MELEE_DAMAGE instead of
                // STAT_UNARMED_DAMAGE.
                int damage = critterGetStat(stack[0], STAT_MELEE_DAMAGE) + 2;
                sprintf(formattedText, "%s 1-%d", messageListItem.text, damage);
            }

            fontDrawText(windowBuffer + offset, formattedText, 120, 499, colorTable[992]);

            offset += 3 * 499 * fontGetLineHeight();
            continue;
        }

        const char* itemName = itemGetName(item);
        fontDrawText(windowBuffer + offset, itemName, 140, 499, colorTable[992]);

        offset += 499 * fontGetLineHeight();

        int itemType = itemGetType(item);
        if (itemType != ITEM_TYPE_WEAPON) {
            if (itemType == ITEM_TYPE_ARMOR) {
                // (Not worn)
                messageListItem.num = 18;
                if (messageListGetItem(&inventry_message_file, &messageListItem)) {
                    fontDrawText(windowBuffer + offset, messageListItem.text, 120, 499, colorTable[992]);
                }
            }

            offset += 3 * 499 * fontGetLineHeight();
            continue;
        }

        int range = _item_w_range(stack[0], hitModes[index]);

        int damageMin;
        int damageMax;
        weaponGetDamageMinMax(item, &damageMin, &damageMax);

        int attackType = weaponGetAttackTypeForHitMode(item, hitModes[index]);

        formattedText[0] = '\0';

        int meleeDamage;
        if (attackType == ATTACK_TYPE_MELEE || attackType == ATTACK_TYPE_UNARMED) {
            meleeDamage = critterGetStat(stack[0], STAT_MELEE_DAMAGE);
        } else {
            meleeDamage = 0;
        }

        messageListItem.num = 15; // Dmg:
        if (messageListGetItem(&inventry_message_file, &messageListItem)) {
            if (attackType != 4 && range <= 1) {
                sprintf(formattedText, "%s %d-%d", messageListItem.text, damageMin, damageMax + meleeDamage);
            } else {
                MessageListItem rangeMessageListItem;
                rangeMessageListItem.num = 16; // Rng:
                if (messageListGetItem(&inventry_message_file, &rangeMessageListItem)) {
                    sprintf(formattedText, "%s %d-%d   %s %d", messageListItem.text, damageMin, damageMax + meleeDamage, rangeMessageListItem.text, range);
                }
            }

            fontDrawText(windowBuffer + offset, formattedText, 140, 499, colorTable[992]);
        }

        offset += 499 * fontGetLineHeight();

        if (ammoGetCapacity(item) > 0) {
            int ammoTypePid = weaponGetAmmoTypePid(item);

            formattedText[0] = '\0';

            messageListItem.num = 17; // Ammo:
            if (messageListGetItem(&inventry_message_file, &messageListItem)) {
                if (ammoTypePid != -1) {
                    if (ammoGetQuantity(item) != 0) {
                        const char* ammoName = protoGetName(ammoTypePid);
                        int capacity = ammoGetCapacity(item);
                        int quantity = ammoGetQuantity(item);
                        sprintf(formattedText, "%s %d/%d %s", messageListItem.text, quantity, capacity, ammoName);
                    } else {
                        int capacity = ammoGetCapacity(item);
                        int quantity = ammoGetQuantity(item);
                        sprintf(formattedText, "%s %d/%d", messageListItem.text, quantity, capacity);
                    }
                }
            } else {
                int capacity = ammoGetCapacity(item);
                int quantity = ammoGetQuantity(item);
                sprintf(formattedText, "%s %d/%d", messageListItem.text, quantity, capacity);
            }

            fontDrawText(windowBuffer + offset, formattedText, 140, 499, colorTable[992]);
        }

        offset += 2 * 499 * fontGetLineHeight();
    }

    // Total wt:
    messageListItem.num = 20;
    if (messageListGetItem(&inventry_message_file, &messageListItem)) {
        if (PID_TYPE(stack[0]->pid) == OBJ_TYPE_CRITTER) {
            int carryWeight = critterGetStat(stack[0], STAT_CARRY_WEIGHT);
            int inventoryWeight = objectGetInventoryWeight(stack[0]);
            sprintf(formattedText, "%s %d/%d", messageListItem.text, inventoryWeight, carryWeight);

            int color = colorTable[992];
            if (critterIsOverloaded(stack[0])) {
                color = colorTable[31744];
            }

            fontDrawText(windowBuffer + offset + 15, formattedText, 120, 499, color);
        } else {
            int inventoryWeight = objectGetInventoryWeight(stack[0]);
            sprintf(formattedText, "%s %d", messageListItem.text, inventoryWeight);

            fontDrawText(windowBuffer + offset + 30, formattedText, 80, 499, colorTable[992]);
        }
    }

    fontSetCurrent(oldFont);
}

// Finds next item of given [itemType] (can be -1 which means any type of
// item).
//
// The [index] is used to control where to continue the search from, -1 - from
// the beginning.
//
// 0x472698
Object* inven_find_type(Object* obj, int itemType, int* indexPtr)
{
    int dummy = -1;
    if (indexPtr == NULL) {
        indexPtr = &dummy;
    }

    *indexPtr += 1;

    Inventory* inventory = &(obj->data.inventory);

    // TODO: Refactor with for loop.
    if (*indexPtr >= inventory->length) {
        return NULL;
    }

    while (itemType != -1 && itemGetType(inventory->items[*indexPtr].item) != itemType) {
        *indexPtr += 1;

        if (*indexPtr >= inventory->length) {
            return NULL;
        }
    }

    return inventory->items[*indexPtr].item;
}

// 0x4726EC
Object* inven_find_id(Object* obj, int id)
{
    if (obj->id == id) {
        return obj;
    }

    Inventory* inventory = &(obj->data.inventory);
    for (int index = 0; index < inventory->length; index++) {
        InventoryItem* inventoryItem = &(inventory->items[index]);
        Object* item = inventoryItem->item;
        if (item->id == id) {
            return item;
        }

        if (itemGetType(item) == ITEM_TYPE_CONTAINER) {
            item = inven_find_id(item, id);
            if (item != NULL) {
                return item;
            }
        }
    }

    return NULL;
}

// 0x472740
Object* inven_index_ptr(Object* obj, int a2)
{
    Inventory* inventory;

    inventory = &(obj->data.inventory);

    if (a2 < 0 || a2 >= inventory->length) {
        return NULL;
    }

    return inventory->items[a2].item;
}

// inven_wield
// 0x472758
int inven_wield(Object* a1, Object* a2, int a3)
{
    return invenWieldFunc(a1, a2, a3, true);
}

// 0x472768
int invenWieldFunc(Object* critter, Object* item, int a3, bool a4)
{
    if (a4) {
        if (!isoIsDisabled()) {
            register_begin(ANIMATION_REQUEST_RESERVED);
        }
    }

    int itemType = itemGetType(item);
    if (itemType == ITEM_TYPE_ARMOR) {
        Object* armor = inven_worn(critter);
        if (armor != NULL) {
            armor->flags &= ~OBJECT_WORN;
        }

        item->flags |= OBJECT_WORN;

        int baseFrmId;
        if (critterGetStat(critter, STAT_GENDER) == GENDER_FEMALE) {
            baseFrmId = armorGetFemaleFid(item);
        } else {
            baseFrmId = armorGetMaleFid(item);
        }

        if (baseFrmId == -1) {
            baseFrmId = 1;
        }

        if (critter == gDude) {
            if (!isoIsDisabled()) {
                int fid = art_id(OBJ_TYPE_CRITTER, baseFrmId, 0, (critter->fid & 0xF000) >> 12, critter->rotation + 1);
                register_object_change_fid(critter, fid, 0);
            }
        } else {
            adjust_ac(critter, armor, item);
        }
    } else {
        int hand;
        if (critter == gDude) {
            hand = intface_is_item_right_hand();
        } else {
            hand = HAND_RIGHT;
        }

        int weaponAnimationCode = weaponGetAnimationCode(item);
        int hitModeAnimationCode = weaponGetAnimationForHitMode(item, HIT_MODE_RIGHT_WEAPON_PRIMARY);
        int fid = art_id(OBJ_TYPE_CRITTER, critter->fid & 0xFFF, hitModeAnimationCode, weaponAnimationCode, critter->rotation + 1);
        if (!art_exists(fid)) {
            debugPrint("\ninven_wield failed!  ERROR ERROR ERROR!");
            return -1;
        }

        Object* v17;
        if (a3) {
            v17 = inven_right_hand(critter);
            item->flags |= OBJECT_IN_RIGHT_HAND;
        } else {
            v17 = inven_left_hand(critter);
            item->flags |= OBJECT_IN_LEFT_HAND;
        }

        Rect rect;
        if (v17 != NULL) {
            v17->flags &= ~OBJECT_IN_ANY_HAND;

            if (v17->pid == PROTO_ID_LIT_FLARE) {
                int lightIntensity;
                int lightDistance;
                if (critter == gDude) {
                    lightIntensity = LIGHT_LEVEL_MAX;
                    lightDistance = 4;
                } else {
                    Proto* proto;
                    if (protoGetProto(critter->pid, &proto) == -1) {
                        return -1;
                    }

                    lightDistance = proto->lightDistance;
                    lightIntensity = proto->lightIntensity;
                }

                objectSetLight(critter, lightDistance, lightIntensity, &rect);
            }
        }

        if (item->pid == PROTO_ID_LIT_FLARE) {
            int lightDistance = item->lightDistance;
            if (lightDistance < critter->lightDistance) {
                lightDistance = critter->lightDistance;
            }

            int lightIntensity = item->lightIntensity;
            if (lightIntensity < critter->lightIntensity) {
                lightIntensity = critter->lightIntensity;
            }

            objectSetLight(critter, lightDistance, lightIntensity, &rect);
            tileWindowRefreshRect(&rect, gElevation);
        }

        if (itemGetType(item) == ITEM_TYPE_WEAPON) {
            weaponAnimationCode = weaponGetAnimationCode(item);
        } else {
            weaponAnimationCode = 0;
        }

        if (hand == a3) {
            if ((critter->fid & 0xF000) >> 12 != 0) {
                if (a4) {
                    if (!isoIsDisabled()) {
                        const char* soundEffectName = gsnd_build_character_sfx_name(critter, ANIM_PUT_AWAY, CHARACTER_SOUND_EFFECT_UNUSED);
                        register_object_play_sfx(critter, soundEffectName, 0);
                        register_object_animate(critter, ANIM_PUT_AWAY, 0);
                    }
                }
            }

            if (a4 && !isoIsDisabled()) {
                if (weaponAnimationCode != 0) {
                    register_object_take_out(critter, weaponAnimationCode, -1);
                } else {
                    int fid = art_id(OBJ_TYPE_CRITTER, critter->fid & 0xFFF, 0, 0, critter->rotation + 1);
                    register_object_change_fid(critter, fid, -1);
                }
            } else {
                int fid = art_id(OBJ_TYPE_CRITTER, critter->fid & 0xFFF, 0, weaponAnimationCode, critter->rotation + 1);
                dude_stand(critter, critter->rotation, fid);
            }
        }
    }

    if (a4) {
        if (!isoIsDisabled()) {
            return register_end();
        }
    }

    return 0;
}

// inven_unwield
// 0x472A54
int inven_unwield(Object* critter_obj, int a2)
{
    return invenUnwieldFunc(critter_obj, a2, 1);
}

// 0x472A64
int invenUnwieldFunc(Object* obj, int a2, int a3)
{
    int v6;
    Object* item_obj;
    int fid;

    if (obj == gDude) {
        v6 = intface_is_item_right_hand();
    } else {
        v6 = 1;
    }

    if (a2) {
        item_obj = inven_right_hand(obj);
    } else {
        item_obj = inven_left_hand(obj);
    }

    if (item_obj) {
        item_obj->flags &= ~OBJECT_IN_ANY_HAND;
    }

    if (v6 == a2 && ((obj->fid & 0xF000) >> 12) != 0) {
        if (a3 && !isoIsDisabled()) {
            register_begin(ANIMATION_REQUEST_RESERVED);

            const char* sfx = gsnd_build_character_sfx_name(obj, ANIM_PUT_AWAY, CHARACTER_SOUND_EFFECT_UNUSED);
            register_object_play_sfx(obj, sfx, 0);

            register_object_animate(obj, ANIM_PUT_AWAY, 0);

            fid = art_id(OBJ_TYPE_CRITTER, obj->fid & 0xFFF, 0, 0, obj->rotation + 1);
            register_object_change_fid(obj, fid, -1);

            return register_end();
        }

        fid = art_id(OBJ_TYPE_CRITTER, obj->fid & 0xFFF, 0, 0, obj->rotation + 1);
        dude_stand(obj, obj->rotation, fid);
    }

    return 0;
}

// 0x472B54
int inven_from_button(int keyCode, Object** a2, Object*** a3, Object** a4)
{
    Object** v6;
    Object* v7;
    Object* v8;
    int quantity = 0;

    switch (keyCode) {
    case 1006:
        v6 = &i_rhand;
        v7 = stack[0];
        v8 = i_rhand;
        break;
    case 1007:
        v6 = &i_lhand;
        v7 = stack[0];
        v8 = i_lhand;
        break;
    case 1008:
        v6 = &i_worn;
        v7 = stack[0];
        v8 = i_worn;
        break;
    default:
        v6 = NULL;
        v7 = NULL;
        v8 = NULL;

        InventoryItem* inventoryItem;
        if (keyCode < 2000) {
            int index = stack_offset[curr_stack] + keyCode - 1000;
            if (index >= pud->length) {
                break;
            }

            inventoryItem = &(pud->items[pud->length - (index + 1)]);
            v8 = inventoryItem->item;
            v7 = stack[curr_stack];
        } else if (keyCode < 2300) {
            int index = target_stack_offset[target_curr_stack] + keyCode - 2000;
            if (index >= target_pud->length) {
                break;
            }

            inventoryItem = &(target_pud->items[target_pud->length - (index + 1)]);
            v8 = inventoryItem->item;
            v7 = target_stack[target_curr_stack];
        } else if (keyCode < 2400) {
            int index = ptable_offset + keyCode - 2300;
            if (index >= ptable_pud->length) {
                break;
            }

            inventoryItem = &(ptable_pud->items[ptable_pud->length - (index + 1)]);
            v8 = inventoryItem->item;
            v7 = ptable;
        } else {
            int index = btable_offset + keyCode - 2400;
            if (index >= btable_pud->length) {
                break;
            }

            inventoryItem = &(btable_pud->items[btable_pud->length - (index + 1)]);
            v8 = inventoryItem->item;
            v7 = btable;
        }

        quantity = inventoryItem->quantity;
    }

    if (a3 != NULL) {
        *a3 = v6;
    }

    if (a2 != NULL) {
        *a2 = v8;
    }

    if (a4 != NULL) {
        *a4 = v7;
    }

    if (quantity == 0 && v8 != NULL) {
        quantity = 1;
    }

    return quantity;
}

// Displays item description.
//
// The [string] is mutated in the process replacing spaces back and forth
// for word wrapping purposes.
//
// inven_display_msg
// 0x472D24
void inven_display_msg(char* string)
{
    int oldFont = fontGetCurrent();
    fontSetCurrent(101);

    unsigned char* windowBuffer = windowGetBuffer(i_wid);
    windowBuffer += 499 * 44 + 297;

    char* c = string;
    while (c != NULL && *c != '\0') {
        inven_display_msg_line += 1;
        if (inven_display_msg_line > 17) {
            debugPrint("\nError: inven_display_msg: out of bounds!");
            return;
        }

        char* space = NULL;
        if (fontGetStringWidth(c) > 152) {
            // Look for next space.
            space = c + 1;
            while (*space != '\0' && *space != ' ') {
                space += 1;
            }

            if (*space == '\0') {
                // This was the last line containing very long word. Text
                // drawing routine will silently truncate it after reaching
                // desired length.
                fontDrawText(windowBuffer + 499 * inven_display_msg_line * fontGetLineHeight(), c, 152, 499, colorTable[992]);
                return;
            }

            char* nextSpace = space + 1;
            while (true) {
                while (*nextSpace != '\0' && *nextSpace != ' ') {
                    nextSpace += 1;
                }

                if (*nextSpace == '\0') {
                    break;
                }

                // Break string and measure it.
                *nextSpace = '\0';
                if (fontGetStringWidth(c) >= 152) {
                    // Next space is too far to fit in one line. Restore next
                    // space's character and stop.
                    *nextSpace = ' ';
                    break;
                }

                space = nextSpace;

                // Restore next space's character and continue looping from the
                // next character.
                *nextSpace = ' ';
                nextSpace += 1;
            }

            if (*space == ' ') {
                *space = '\0';
            }
        }

        if (fontGetStringWidth(c) > 152) {
            debugPrint("\nError: inven_display_msg: word too long!");
            return;
        }

        fontDrawText(windowBuffer + 499 * inven_display_msg_line * fontGetLineHeight(), c, 152, 499, colorTable[992]);

        if (space != NULL) {
            c = space + 1;
            if (*space == '\0') {
                *space = ' ';
            }
        } else {
            c = NULL;
        }
    }

    fontSetCurrent(oldFont);
}

// Examines inventory item.
//
// 0x472EB8
void inven_obj_examine_func(Object* critter, Object* item)
{
    int oldFont = fontGetCurrent();
    fontSetCurrent(101);

    unsigned char* windowBuffer = windowGetBuffer(i_wid);

    // Clear item description area.
    int backgroundFid = art_id(OBJ_TYPE_INTERFACE, 48, 0, 0, 0);

    CacheEntry* handle;
    unsigned char* backgroundData = art_ptr_lock_data(backgroundFid, 0, 0, &handle);
    if (backgroundData != NULL) {
        blitBufferToBuffer(backgroundData + 499 * 44 + 297, 152, 188, 499, windowBuffer + 499 * 44 + 297, 499);
    }
    art_ptr_unlock(handle);

    // Reset item description lines counter.
    inven_display_msg_line = 0;

    // Render item's name.
    char* itemName = objectGetName(item);
    inven_display_msg(itemName);

    // Increment line counter to accomodate separator below.
    inven_display_msg_line += 1;

    int lineHeight = fontGetLineHeight();

    // Draw separator.
    bufferDrawLine(windowBuffer,
        499,
        297,
        3 * lineHeight / 2 + 49,
        440,
        3 * lineHeight / 2 + 49,
        colorTable[992]);

    // Examine item.
    _obj_examine_func(critter, item, inven_display_msg);

    // Add weight if neccessary.
    int weight = itemGetWeight(item);
    if (weight != 0) {
        MessageListItem messageListItem;
        messageListItem.num = 540;

        if (weight == 1) {
            messageListItem.num = 541;
        }

        if (!messageListGetItem(&gProtoMessageList, &messageListItem)) {
            debugPrint("\nError: Couldn't find message!");
        }

        char formattedText[40];
        sprintf(formattedText, messageListItem.text, weight);
        inven_display_msg(formattedText);
    }

    fontSetCurrent(oldFont);
}

// 0x47304C
void inven_action_cursor(int keyCode, int inventoryWindowType)
{
    // 0x519114
    static int act_use[4] = {
        GAME_MOUSE_ACTION_MENU_ITEM_LOOK,
        GAME_MOUSE_ACTION_MENU_ITEM_USE,
        GAME_MOUSE_ACTION_MENU_ITEM_DROP,
        GAME_MOUSE_ACTION_MENU_ITEM_CANCEL,
    };

    // 0x519124
    static int act_no_use[3] = {
        GAME_MOUSE_ACTION_MENU_ITEM_LOOK,
        GAME_MOUSE_ACTION_MENU_ITEM_DROP,
        GAME_MOUSE_ACTION_MENU_ITEM_CANCEL,
    };

    // 0x519130
    static int act_just_use[3] = {
        GAME_MOUSE_ACTION_MENU_ITEM_LOOK,
        GAME_MOUSE_ACTION_MENU_ITEM_USE,
        GAME_MOUSE_ACTION_MENU_ITEM_CANCEL,
    };

    // 0x51913C
    static int act_nothing[2] = {
        GAME_MOUSE_ACTION_MENU_ITEM_LOOK,
        GAME_MOUSE_ACTION_MENU_ITEM_CANCEL,
    };

    // 0x519144
    static int act_weap[4] = {
        GAME_MOUSE_ACTION_MENU_ITEM_LOOK,
        GAME_MOUSE_ACTION_MENU_ITEM_UNLOAD,
        GAME_MOUSE_ACTION_MENU_ITEM_DROP,
        GAME_MOUSE_ACTION_MENU_ITEM_CANCEL,
    };

    // 0x519154
    static int act_weap2[3] = {
        GAME_MOUSE_ACTION_MENU_ITEM_LOOK,
        GAME_MOUSE_ACTION_MENU_ITEM_UNLOAD,
        GAME_MOUSE_ACTION_MENU_ITEM_CANCEL,
    };

    Object* item;
    Object** v43;
    Object* v41;

    int v56 = inven_from_button(keyCode, &item, &v43, &v41);
    if (v56 == 0) {
        return;
    }

    int itemType = itemGetType(item);

    int mouseState;
    do {
        _get_input();

        if (inventoryWindowType == INVENTORY_WINDOW_TYPE_NORMAL) {
            display_body(-1, INVENTORY_WINDOW_TYPE_NORMAL);
        }

        mouseState = mouse_get_buttons();
        if ((mouseState & MOUSE_EVENT_LEFT_BUTTON_UP) != 0) {
            if (inventoryWindowType != INVENTORY_WINDOW_TYPE_NORMAL) {
                _obj_look_at_func(stack[0], item, display_msg);
            } else {
                inven_obj_examine_func(stack[0], item);
            }
            win_draw(i_wid);
            return;
        }
    } while ((mouseState & MOUSE_EVENT_LEFT_BUTTON_DOWN_REPEAT) != MOUSE_EVENT_LEFT_BUTTON_DOWN_REPEAT);

    inven_set_mouse(INVENTORY_WINDOW_CURSOR_BLANK);

    unsigned char* windowBuffer = windowGetBuffer(i_wid);

    int x;
    int y;
    mouse_get_position(&x, &y);

    int actionMenuItemsLength;
    const int* actionMenuItems;
    if (itemType == ITEM_TYPE_WEAPON && _item_w_can_unload(item)) {
        if (inventoryWindowType != INVENTORY_WINDOW_TYPE_NORMAL && objectGetOwner(item) != gDude) {
            actionMenuItemsLength = 3;
            actionMenuItems = act_weap2;
        } else {
            actionMenuItemsLength = 4;
            actionMenuItems = act_weap;
        }
    } else {
        if (inventoryWindowType != INVENTORY_WINDOW_TYPE_NORMAL) {
            if (objectGetOwner(item) != gDude) {
                if (itemType == ITEM_TYPE_CONTAINER) {
                    actionMenuItemsLength = 3;
                    actionMenuItems = act_just_use;
                } else {
                    actionMenuItemsLength = 2;
                    actionMenuItems = act_nothing;
                }
            } else {
                if (itemType == ITEM_TYPE_CONTAINER) {
                    actionMenuItemsLength = 4;
                    actionMenuItems = act_use;
                } else {
                    actionMenuItemsLength = 3;
                    actionMenuItems = act_no_use;
                }
            }
        } else {
            if (itemType == ITEM_TYPE_CONTAINER && v43 != NULL) {
                actionMenuItemsLength = 3;
                actionMenuItems = act_no_use;
            } else {
                if (_obj_action_can_use(item) || _proto_action_can_use_on(item->pid)) {
                    actionMenuItemsLength = 4;
                    actionMenuItems = act_use;
                } else {
                    actionMenuItemsLength = 3;
                    actionMenuItems = act_no_use;
                }
            }
        }
    }

    InventoryWindowDescription* windowDescription = &(iscr_data[inventoryWindowType]);
    gmouse_3d_build_menu_frame(x, y, actionMenuItems, actionMenuItemsLength,
        windowDescription->width + windowDescription->x,
        windowDescription->height + windowDescription->y);

    InventoryCursorData* cursorData = &(imdata[INVENTORY_WINDOW_CURSOR_MENU]);

    int offsetX;
    int offsetY;
    art_frame_offset(cursorData->frm, 0, &offsetX, &offsetY);

    Rect rect;
    rect.left = x - windowDescription->x - cursorData->width / 2 + offsetX;
    rect.top = y - windowDescription->y - cursorData->height + 1 + offsetY;
    rect.right = rect.left + cursorData->width - 1;
    rect.bottom = rect.top + cursorData->height - 1;

    int menuButtonHeight = cursorData->height;
    if (rect.top + menuButtonHeight > windowDescription->height) {
        menuButtonHeight = windowDescription->height - rect.top;
    }

    int btn = buttonCreate(i_wid,
        rect.left,
        rect.top,
        cursorData->width,
        menuButtonHeight,
        -1,
        -1,
        -1,
        -1,
        cursorData->frmData,
        cursorData->frmData,
        0,
        BUTTON_FLAG_TRANSPARENT);
    win_draw_rect(i_wid, &rect);

    int menuItemIndex = 0;
    int previousMouseY = y;
    while ((mouse_get_buttons() & MOUSE_EVENT_LEFT_BUTTON_UP) == 0) {
        _get_input();

        if (inventoryWindowType == INVENTORY_WINDOW_TYPE_NORMAL) {
            display_body(-1, INVENTORY_WINDOW_TYPE_NORMAL);
        }

        int x;
        int y;
        mouse_get_position(&x, &y);
        if (y - previousMouseY > 10 || previousMouseY - y > 10) {
            if (y >= previousMouseY || menuItemIndex <= 0) {
                if (previousMouseY < y && menuItemIndex < actionMenuItemsLength - 1) {
                    menuItemIndex++;
                }
            } else {
                menuItemIndex--;
            }
            gmouse_3d_highlight_menu_frame(menuItemIndex);
            win_draw_rect(i_wid, &rect);
            previousMouseY = y;
        }
    }

    buttonDestroy(btn);

    if (inventoryWindowType == INVENTORY_WINDOW_TYPE_TRADE) {
        unsigned char* src = windowGetBuffer(barter_back_win);
        int pitch = _scr_size.right - _scr_size.left + 1;
        blitBufferToBuffer(src + pitch * rect.top + rect.left + 80,
            cursorData->width,
            menuButtonHeight,
            pitch,
            windowBuffer + windowDescription->width * rect.top + rect.left,
            windowDescription->width);
    } else {
        int backgroundFid = art_id(OBJ_TYPE_INTERFACE, windowDescription->field_0, 0, 0, 0);
        CacheEntry* backgroundFrmHandle;
        unsigned char* backgroundFrmData = art_ptr_lock_data(backgroundFid, 0, 0, &backgroundFrmHandle);
        blitBufferToBuffer(backgroundFrmData + windowDescription->width * rect.top + rect.left,
            cursorData->width,
            menuButtonHeight,
            windowDescription->width,
            windowBuffer + windowDescription->width * rect.top + rect.left,
            windowDescription->width);
        art_ptr_unlock(backgroundFrmHandle);
    }

    mouse_set_position(x, y);

    display_inventory(stack_offset[curr_stack], -1, inventoryWindowType);

    int actionMenuItem = actionMenuItems[menuItemIndex];
    switch (actionMenuItem) {
    case GAME_MOUSE_ACTION_MENU_ITEM_DROP:
        if (v43 != NULL) {
            if (v43 == &i_worn) {
                adjust_ac(stack[0], item, NULL);
            }
            itemAdd(v41, item, 1);
            v56 = 1;
            *v43 = NULL;
        }

        if (item->pid == PROTO_ID_MONEY) {
            if (v56 > 1) {
                v56 = do_move_timer(INVENTORY_WINDOW_TYPE_MOVE_ITEMS, item, v56);
            } else {
                v56 = 1;
            }

            if (v56 > 0) {
                if (v56 == 1) {
                    itemSetMoney(item, 1);
                    _obj_drop(v41, item);
                } else {
                    if (itemRemove(v41, item, v56 - 1) == 0) {
                        Object* a2;
                        if (inven_from_button(keyCode, &a2, &v43, &v41) != 0) {
                            itemSetMoney(a2, v56);
                            _obj_drop(v41, a2);
                        } else {
                            itemAdd(v41, item, v56 - 1);
                        }
                    }
                }
            }
        } else if (item->pid == PROTO_ID_DYNAMITE_II || item->pid == PROTO_ID_PLASTIC_EXPLOSIVES_II) {
            dropped_explosive = 1;
            _obj_drop(v41, item);
        } else {
            if (v56 > 1) {
                v56 = do_move_timer(INVENTORY_WINDOW_TYPE_MOVE_ITEMS, item, v56);

                for (int index = 0; index < v56; index++) {
                    if (inven_from_button(keyCode, &item, &v43, &v41) != 0) {
                        _obj_drop(v41, item);
                    }
                }
            } else {
                _obj_drop(v41, item);
            }
        }
        break;
    case GAME_MOUSE_ACTION_MENU_ITEM_LOOK:
        if (inventoryWindowType != INVENTORY_WINDOW_TYPE_NORMAL) {
            _obj_examine_func(stack[0], item, display_msg);
        } else {
            inven_obj_examine_func(stack[0], item);
        }
        break;
    case GAME_MOUSE_ACTION_MENU_ITEM_USE:
        switch (itemType) {
        case ITEM_TYPE_CONTAINER:
            container_enter(keyCode, inventoryWindowType);
            break;
        case ITEM_TYPE_DRUG:
            if (_item_d_take_drug(stack[0], item)) {
                if (v43 != NULL) {
                    *v43 = NULL;
                } else {
                    itemRemove(v41, item, 1);
                }

                _obj_connect(item, gDude->tile, gDude->elevation, NULL);
                _obj_destroy(item);
            }
            intface_update_hit_points(true);
            break;
        case ITEM_TYPE_WEAPON:
        case ITEM_TYPE_MISC:
            if (v43 == NULL) {
                itemRemove(v41, item, 1);
            }

            int v21;
            if (_obj_action_can_use(item)) {
                v21 = _protinst_use_item(stack[0], item);
            } else {
                v21 = _protinst_use_item_on(stack[0], stack[0], item);
            }

            if (v21 == 1) {
                if (v43 != NULL) {
                    *v43 = NULL;
                }

                _obj_connect(item, gDude->tile, gDude->elevation, NULL);
                _obj_destroy(item);
            } else {
                if (v43 == NULL) {
                    itemAdd(v41, item, 1);
                }
            }
        }
        break;
    case GAME_MOUSE_ACTION_MENU_ITEM_UNLOAD:
        if (v43 == NULL) {
            itemRemove(v41, item, 1);
        }

        for (;;) {
            Object* ammo = _item_w_unload(item);
            if (ammo == NULL) {
                break;
            }

            Rect rect;
            _obj_disconnect(ammo, &rect);
            itemAdd(v41, ammo, 1);
        }

        if (v43 == NULL) {
            itemAdd(v41, item, 1);
        }
        break;
    default:
        break;
    }

    inven_set_mouse(INVENTORY_WINDOW_CURSOR_ARROW);

    if (inventoryWindowType == INVENTORY_WINDOW_TYPE_NORMAL && actionMenuItem != GAME_MOUSE_ACTION_MENU_ITEM_LOOK) {
        display_stats();
    }

    if (inventoryWindowType == INVENTORY_WINDOW_TYPE_LOOT
        || inventoryWindowType == INVENTORY_WINDOW_TYPE_TRADE) {
        display_target_inventory(target_stack_offset[target_curr_stack], -1, target_pud, inventoryWindowType);
    }

    display_inventory(stack_offset[curr_stack], -1, inventoryWindowType);

    if (inventoryWindowType == INVENTORY_WINDOW_TYPE_TRADE) {
        display_table_inventories(barter_back_win, ptable, btable, -1);
    }

    adjust_fid();
}

// 0x473904
int loot_container(Object* a1, Object* a2)
{
    // 0x46E708
    static const int arrowFrmIds[INVENTORY_ARROW_FRM_COUNT] = {
        122, // left arrow up
        123, // left arrow down
        124, // right arrow up
        125, // right arrow down
    };

    CacheEntry* arrowFrmHandles[INVENTORY_ARROW_FRM_COUNT];
    MessageListItem messageListItem;

    if (a1 != inven_dude) {
        return 0;
    }

    if (FID_TYPE(a2->fid) == OBJ_TYPE_CRITTER) {
        if (critter_flag_check(a2->pid, CRITTER_NO_STEAL)) {
            // You can't find anything to take from that.
            messageListItem.num = 50;
            if (messageListGetItem(&inventry_message_file, &messageListItem)) {
                display_print(messageListItem.text);
            }
            return 0;
        }
    }

    if (FID_TYPE(a2->fid) == OBJ_TYPE_ITEM) {
        if (itemGetType(a2) == ITEM_TYPE_CONTAINER) {
            if (a2->frame == 0) {
                CacheEntry* handle;
                Art* frm = art_ptr_lock(a2->fid, &handle);
                if (frm != NULL) {
                    int frameCount = art_frame_max_frame(frm);
                    art_ptr_unlock(handle);
                    if (frameCount > 1) {
                        return 0;
                    }
                }
            }
        }
    }

    int sid = -1;
    if (!_gIsSteal) {
        if (_obj_sid(a2, &sid) != -1) {
            scriptSetObjects(sid, a1, NULL);
            scriptExecProc(sid, SCRIPT_PROC_PICKUP);

            Script* script;
            if (scriptGetScript(sid, &script) != -1) {
                if (script->scriptOverrides) {
                    return 0;
                }
            }
        }
    }

    if (inven_init() == -1) {
        return 0;
    }

    target_pud = &(a2->data.inventory);
    target_curr_stack = 0;
    target_stack_offset[0] = 0;
    target_stack[0] = a2;

    Object* a1a = NULL;
    if (objectCreateWithFidPid(&a1a, 0, 467) == -1) {
        return 0;
    }

    _item_move_all_hidden(a2, a1a);

    Object* item1 = NULL;
    Object* item2 = NULL;
    Object* armor = NULL;

    if (_gIsSteal) {
        item1 = inven_left_hand(a2);
        if (item1 != NULL) {
            itemRemove(a2, item1, 1);
        }

        item2 = inven_right_hand(a2);
        if (item2 != NULL) {
            itemRemove(a2, item2, 1);
        }

        armor = inven_worn(a2);
        if (armor != NULL) {
            itemRemove(a2, armor, 1);
        }
    }

    bool isoWasEnabled = setup_inventory(INVENTORY_WINDOW_TYPE_LOOT);

    Object** critters = NULL;
    int critterCount = 0;
    int critterIndex = 0;
    if (!_gIsSteal) {
        if (FID_TYPE(a2->fid) == OBJ_TYPE_CRITTER) {
            critterCount = objectListCreate(a2->tile, a2->elevation, OBJ_TYPE_CRITTER, &critters);
            int endIndex = critterCount - 1;
            for (int index = 0; index < critterCount; index++) {
                Object* critter = critters[index];
                if ((critter->data.critter.combat.results & (DAM_DEAD | DAM_KNOCKED_OUT)) == 0) {
                    critters[index] = critters[endIndex];
                    critters[endIndex] = critter;
                    critterCount--;
                    index--;
                    endIndex--;
                } else {
                    critterIndex++;
                }
            }

            if (critterCount == 1) {
                objectListFree(critters);
                critterCount = 0;
            }

            if (critterCount > 1) {
                int fid;
                unsigned char* buttonUpData;
                unsigned char* buttonDownData;
                int btn;

                for (int index = 0; index < INVENTORY_ARROW_FRM_COUNT; index++) {
                    arrowFrmHandles[index] = INVALID_CACHE_ENTRY;
                }

                // Setup left arrow button.
                fid = art_id(OBJ_TYPE_INTERFACE, arrowFrmIds[INVENTORY_ARROW_FRM_LEFT_ARROW_UP], 0, 0, 0);
                buttonUpData = art_ptr_lock_data(fid, 0, 0, &(arrowFrmHandles[INVENTORY_ARROW_FRM_LEFT_ARROW_UP]));

                fid = art_id(OBJ_TYPE_INTERFACE, arrowFrmIds[INVENTORY_ARROW_FRM_LEFT_ARROW_DOWN], 0, 0, 0);
                buttonDownData = art_ptr_lock_data(fid, 0, 0, &(arrowFrmHandles[INVENTORY_ARROW_FRM_LEFT_ARROW_DOWN]));

                if (buttonUpData != NULL && buttonDownData != NULL) {
                    btn = buttonCreate(i_wid, 436, 162, 20, 18, -1, -1, KEY_PAGE_UP, -1, buttonUpData, buttonDownData, NULL, 0);
                    if (btn != -1) {
                        buttonSetCallbacks(btn, gsound_red_butt_press, gsound_red_butt_release);
                    }
                }

                // Setup right arrow button.
                fid = art_id(OBJ_TYPE_INTERFACE, arrowFrmIds[INVENTORY_ARROW_FRM_RIGHT_ARROW_UP], 0, 0, 0);
                buttonUpData = art_ptr_lock_data(fid, 0, 0, &(arrowFrmHandles[INVENTORY_ARROW_FRM_RIGHT_ARROW_UP]));

                fid = art_id(OBJ_TYPE_INTERFACE, arrowFrmIds[INVENTORY_ARROW_FRM_RIGHT_ARROW_DOWN], 0, 0, 0);
                buttonDownData = art_ptr_lock_data(fid, 0, 0, &(arrowFrmHandles[INVENTORY_ARROW_FRM_RIGHT_ARROW_DOWN]));

                if (buttonUpData != NULL && buttonDownData != NULL) {
                    btn = buttonCreate(i_wid, 456, 162, 20, 18, -1, -1, KEY_PAGE_DOWN, -1, buttonUpData, buttonDownData, NULL, 0);
                    if (btn != -1) {
                        buttonSetCallbacks(btn, gsound_red_butt_press, gsound_red_butt_release);
                    }
                }

                for (int index = 0; index < critterCount; index++) {
                    if (a2 == critters[index]) {
                        critterIndex = index;
                    }
                }
            }
        }
    }

    display_target_inventory(target_stack_offset[target_curr_stack], -1, target_pud, INVENTORY_WINDOW_TYPE_LOOT);
    display_inventory(stack_offset[curr_stack], -1, INVENTORY_WINDOW_TYPE_LOOT);
    display_body(a2->fid, INVENTORY_WINDOW_TYPE_LOOT);
    inven_set_mouse(INVENTORY_WINDOW_CURSOR_HAND);

    bool isCaughtStealing = false;
    int stealingXp = 0;
    int stealingXpBonus = 10;
    for (;;) {
        if (game_user_wants_to_quit != 0) {
            break;
        }

        if (isCaughtStealing) {
            break;
        }

        int keyCode = _get_input();

        if (keyCode == KEY_CTRL_Q || keyCode == KEY_CTRL_X || keyCode == KEY_F10) {
            game_quit_with_confirm();
        }

        if (game_user_wants_to_quit != 0) {
            break;
        }

        if (keyCode == KEY_UPPERCASE_A) {
            if (!_gIsSteal) {
                int maxCarryWeight = critterGetStat(a1, STAT_CARRY_WEIGHT);
                int currentWeight = objectGetInventoryWeight(a1);
                int newInventoryWeight = objectGetInventoryWeight(a2);
                if (newInventoryWeight <= maxCarryWeight - currentWeight) {
                    _item_move_all(a2, a1);
                    display_target_inventory(target_stack_offset[target_curr_stack], -1, target_pud, INVENTORY_WINDOW_TYPE_LOOT);
                    display_inventory(stack_offset[curr_stack], -1, INVENTORY_WINDOW_TYPE_LOOT);
                } else {
                    // Sorry, you cannot carry that much.
                    messageListItem.num = 31;
                    if (messageListGetItem(&inventry_message_file, &messageListItem)) {
                        dialog_out(messageListItem.text, NULL, 0, 169, 117, colorTable[32328], NULL, colorTable[32328], 0);
                    }
                }
            }
        } else if (keyCode == KEY_ARROW_UP) {
            if (stack_offset[curr_stack] > 0) {
                stack_offset[curr_stack] -= 1;
                display_inventory(stack_offset[curr_stack], -1, INVENTORY_WINDOW_TYPE_LOOT);
            }
        } else if (keyCode == KEY_PAGE_UP) {
            if (critterCount != 0) {
                if (critterIndex > 0) {
                    critterIndex -= 1;
                } else {
                    critterIndex = critterCount - 1;
                }

                a2 = critters[critterIndex];
                target_pud = &(a2->data.inventory);
                target_stack[0] = a2;
                target_curr_stack = 0;
                target_stack_offset[0] = 0;
                display_target_inventory(0, -1, target_pud, INVENTORY_WINDOW_TYPE_LOOT);
                display_inventory(stack_offset[curr_stack], -1, INVENTORY_WINDOW_TYPE_LOOT);
                display_body(a2->fid, INVENTORY_WINDOW_TYPE_LOOT);
            }
        } else if (keyCode == KEY_ARROW_DOWN) {
            if (stack_offset[curr_stack] + inven_cur_disp < pud->length) {
                stack_offset[curr_stack] += 1;
                display_inventory(stack_offset[curr_stack], -1, INVENTORY_WINDOW_TYPE_LOOT);
            }
        } else if (keyCode == KEY_PAGE_DOWN) {
            if (critterCount != 0) {
                if (critterIndex < critterCount - 1) {
                    critterIndex += 1;
                } else {
                    critterIndex = 0;
                }

                a2 = critters[critterIndex];
                target_pud = &(a2->data.inventory);
                target_stack[0] = a2;
                target_curr_stack = 0;
                target_stack_offset[0] = 0;
                display_target_inventory(0, -1, target_pud, INVENTORY_WINDOW_TYPE_LOOT);
                display_inventory(stack_offset[curr_stack], -1, INVENTORY_WINDOW_TYPE_LOOT);
                display_body(a2->fid, INVENTORY_WINDOW_TYPE_LOOT);
            }
        } else if (keyCode == KEY_CTRL_ARROW_UP) {
            if (target_stack_offset[target_curr_stack] > 0) {
                target_stack_offset[target_curr_stack] -= 1;
                display_target_inventory(target_stack_offset[target_curr_stack], -1, target_pud, INVENTORY_WINDOW_TYPE_LOOT);
                win_draw(i_wid);
            }
        } else if (keyCode == KEY_CTRL_ARROW_DOWN) {
            if (target_stack_offset[target_curr_stack] + inven_cur_disp < target_pud->length) {
                target_stack_offset[target_curr_stack] += 1;
                display_target_inventory(target_stack_offset[target_curr_stack], -1, target_pud, INVENTORY_WINDOW_TYPE_LOOT);
                win_draw(i_wid);
            }
        } else if (keyCode >= 2500 && keyCode <= 2501) {
            container_exit(keyCode, INVENTORY_WINDOW_TYPE_LOOT);
        } else {
            if ((mouse_get_buttons() & MOUSE_EVENT_RIGHT_BUTTON_DOWN) != 0) {
                if (immode == INVENTORY_WINDOW_CURSOR_HAND) {
                    inven_set_mouse(INVENTORY_WINDOW_CURSOR_ARROW);
                } else {
                    inven_set_mouse(INVENTORY_WINDOW_CURSOR_HAND);
                }
            } else if ((mouse_get_buttons() & MOUSE_EVENT_LEFT_BUTTON_DOWN) != 0) {
                if (keyCode >= 1000 && keyCode <= 1000 + inven_cur_disp) {
                    if (immode == INVENTORY_WINDOW_CURSOR_ARROW) {
                        inven_action_cursor(keyCode, INVENTORY_WINDOW_TYPE_LOOT);
                    } else {
                        int v40 = keyCode - 1000;
                        if (v40 + stack_offset[curr_stack] < pud->length) {
                            _gStealCount += 1;
                            _gStealSize += itemGetSize(stack[curr_stack]);

                            InventoryItem* inventoryItem = &(pud->items[pud->length - (v40 + stack_offset[curr_stack] + 1)]);
                            int rc = move_inventory(inventoryItem->item, v40, target_stack[target_curr_stack], true);
                            if (rc == 1) {
                                isCaughtStealing = true;
                            } else if (rc == 2) {
                                stealingXp += stealingXpBonus;
                                stealingXpBonus += 10;
                            }

                            display_target_inventory(target_stack_offset[target_curr_stack], -1, target_pud, INVENTORY_WINDOW_TYPE_LOOT);
                            display_inventory(stack_offset[curr_stack], -1, INVENTORY_WINDOW_TYPE_LOOT);
                        }

                        keyCode = -1;
                    }
                } else if (keyCode >= 2000 && keyCode <= 2000 + inven_cur_disp) {
                    if (immode == INVENTORY_WINDOW_CURSOR_ARROW) {
                        inven_action_cursor(keyCode, INVENTORY_WINDOW_TYPE_LOOT);
                    } else {
                        int v46 = keyCode - 2000;
                        if (v46 + target_stack_offset[target_curr_stack] < target_pud->length) {
                            _gStealCount += 1;
                            _gStealSize += itemGetSize(stack[curr_stack]);

                            InventoryItem* inventoryItem = &(target_pud->items[target_pud->length - (v46 + target_stack_offset[target_curr_stack] + 1)]);
                            int rc = move_inventory(inventoryItem->item, v46, target_stack[target_curr_stack], false);
                            if (rc == 1) {
                                isCaughtStealing = true;
                            } else if (rc == 2) {
                                stealingXp += stealingXpBonus;
                                stealingXpBonus += 10;
                            }

                            display_target_inventory(target_stack_offset[target_curr_stack], -1, target_pud, INVENTORY_WINDOW_TYPE_LOOT);
                            display_inventory(stack_offset[curr_stack], -1, INVENTORY_WINDOW_TYPE_LOOT);
                        }
                    }
                }
            }
        }

        if (keyCode == KEY_ESCAPE) {
            break;
        }
    }

    if (critterCount != 0) {
        objectListFree(critters);

        for (int index = 0; index < INVENTORY_ARROW_FRM_COUNT; index++) {
            art_ptr_unlock(arrowFrmHandles[index]);
        }
    }

    if (_gIsSteal) {
        if (item1 != NULL) {
            item1->flags |= OBJECT_IN_LEFT_HAND;
            itemAdd(a2, item1, 1);
        }

        if (item2 != NULL) {
            item2->flags |= OBJECT_IN_RIGHT_HAND;
            itemAdd(a2, item2, 1);
        }

        if (armor != NULL) {
            armor->flags |= OBJECT_WORN;
            itemAdd(a2, armor, 1);
        }
    }

    _item_move_all(a1a, a2);
    objectDestroy(a1a, NULL);

    if (_gIsSteal) {
        if (!isCaughtStealing) {
            if (stealingXp > 0) {
                if (!objectIsPartyMember(a2)) {
                    stealingXp = min(300 - skillGetValue(a1, SKILL_STEAL), stealingXp);
                    debugPrint("\n[[[%d]]]", 300 - skillGetValue(a1, SKILL_STEAL));

                    // You gain %d experience points for successfully using your Steal skill.
                    messageListItem.num = 29;
                    if (messageListGetItem(&inventry_message_file, &messageListItem)) {
                        char formattedText[200];
                        sprintf(formattedText, messageListItem.text, stealingXp);
                        display_print(formattedText);
                    }

                    pcAddExperience(stealingXp);
                }
            }
        }
    }

    exit_inventory(isoWasEnabled);

    // NOTE: Uninline.
    inven_exit();

    if (_gIsSteal) {
        if (isCaughtStealing) {
            if (_gStealCount > 0) {
                if (_obj_sid(a2, &sid) != -1) {
                    scriptSetObjects(sid, a1, NULL);
                    scriptExecProc(sid, SCRIPT_PROC_PICKUP);

                    // TODO: Looks like inlining, script is not used.
                    Script* script;
                    scriptGetScript(sid, &script);
                }
            }
        }
    }

    return 0;
}

// 0x4746A0
int inven_steal_container(Object* a1, Object* a2)
{
    if (a1 == a2) {
        return -1;
    }

    _gIsSteal = PID_TYPE(a1->pid) == OBJ_TYPE_CRITTER && critter_is_active(a2);
    _gStealCount = 0;
    _gStealSize = 0;

    int rc = loot_container(a1, a2);

    _gIsSteal = 0;
    _gStealCount = 0;
    _gStealSize = 0;

    return rc;
}

// 0x474708
int move_inventory(Object* a1, int a2, Object* a3, bool a4)
{
    bool v38 = true;

    Rect rect;

    int quantity;
    if (a4) {
        rect.left = INVENTORY_LOOT_LEFT_SCROLLER_X;
        rect.top = INVENTORY_SLOT_HEIGHT * a2 + INVENTORY_LOOT_LEFT_SCROLLER_Y;

        InventoryItem* inventoryItem = &(pud->items[pud->length - (a2 + stack_offset[curr_stack] + 1)]);
        quantity = inventoryItem->quantity;
        if (quantity > 1) {
            display_inventory(stack_offset[curr_stack], a2, INVENTORY_WINDOW_TYPE_LOOT);
            v38 = false;
        }
    } else {
        rect.left = INVENTORY_LOOT_RIGHT_SCROLLER_X;
        rect.top = INVENTORY_SLOT_HEIGHT * a2 + INVENTORY_LOOT_RIGHT_SCROLLER_Y;

        InventoryItem* inventoryItem = &(target_pud->items[target_pud->length - (a2 + target_stack_offset[target_curr_stack] + 1)]);
        quantity = inventoryItem->quantity;
        if (quantity > 1) {
            display_target_inventory(target_stack_offset[target_curr_stack], a2, target_pud, INVENTORY_WINDOW_TYPE_LOOT);
            win_draw(i_wid);
            v38 = false;
        }
    }

    if (v38) {
        unsigned char* windowBuffer = windowGetBuffer(i_wid);

        CacheEntry* handle;
        int fid = art_id(OBJ_TYPE_INTERFACE, 114, 0, 0, 0);
        unsigned char* data = art_ptr_lock_data(fid, 0, 0, &handle);
        if (data != NULL) {
            blitBufferToBuffer(data + 537 * rect.top + rect.left, INVENTORY_SLOT_WIDTH, INVENTORY_SLOT_HEIGHT, 537, windowBuffer + 537 * rect.top + rect.left, 537);
            art_ptr_unlock(handle);
        }

        rect.right = rect.left + INVENTORY_SLOT_WIDTH - 1;
        rect.bottom = rect.top + INVENTORY_SLOT_HEIGHT - 1;
        win_draw_rect(i_wid, &rect);
    }

    CacheEntry* inventoryFrmHandle;
    int inventoryFid = itemGetInventoryFid(a1);
    Art* inventoryFrm = art_ptr_lock(inventoryFid, &inventoryFrmHandle);
    if (inventoryFrm != NULL) {
        int width = art_frame_width(inventoryFrm, 0, 0);
        int height = art_frame_length(inventoryFrm, 0, 0);
        unsigned char* data = art_frame_data(inventoryFrm, 0, 0);
        mouse_set_shape(data, width, height, width, width / 2, height / 2, 0);
        gsound_play_sfx_file("ipickup1");
    }

    do {
        _get_input();
    } while ((mouse_get_buttons() & MOUSE_EVENT_LEFT_BUTTON_REPEAT) != 0);

    if (inventoryFrm != NULL) {
        art_ptr_unlock(inventoryFrmHandle);
        gsound_play_sfx_file("iputdown");
    }

    int rc = 0;
    MessageListItem messageListItem;

    if (a4) {
        if (mouse_click_in(INVENTORY_LOOT_RIGHT_SCROLLER_ABS_X, INVENTORY_LOOT_RIGHT_SCROLLER_ABS_Y, INVENTORY_LOOT_RIGHT_SCROLLER_ABS_MAX_X, INVENTORY_SLOT_HEIGHT * inven_cur_disp + INVENTORY_LOOT_RIGHT_SCROLLER_ABS_Y)) {
            int quantityToMove;
            if (quantity > 1) {
                quantityToMove = do_move_timer(INVENTORY_WINDOW_TYPE_MOVE_ITEMS, a1, quantity);
            } else {
                quantityToMove = 1;
            }

            if (quantityToMove != -1) {
                if (_gIsSteal) {
                    if (skillsPerformStealing(inven_dude, a3, a1, true) == 0) {
                        rc = 1;
                    }
                }

                if (rc != 1) {
                    if (_item_move(inven_dude, a3, a1, quantityToMove) != -1) {
                        rc = 2;
                    } else {
                        // There is no space left for that item.
                        messageListItem.num = 26;
                        if (messageListGetItem(&inventry_message_file, &messageListItem)) {
                            display_print(messageListItem.text);
                        }
                    }
                }
            }
        }
    } else {
        if (mouse_click_in(INVENTORY_LOOT_LEFT_SCROLLER_ABS_X, INVENTORY_LOOT_LEFT_SCROLLER_ABS_Y, INVENTORY_LOOT_LEFT_SCROLLER_ABS_MAX_X, INVENTORY_SLOT_HEIGHT * inven_cur_disp + INVENTORY_LOOT_LEFT_SCROLLER_ABS_Y)) {
            int quantityToMove;
            if (quantity > 1) {
                quantityToMove = do_move_timer(INVENTORY_WINDOW_TYPE_MOVE_ITEMS, a1, quantity);
            } else {
                quantityToMove = 1;
            }

            if (quantityToMove != -1) {
                if (_gIsSteal) {
                    if (skillsPerformStealing(inven_dude, a3, a1, false) == 0) {
                        rc = 1;
                    }
                }

                if (rc != 1) {
                    if (_item_move(a3, inven_dude, a1, quantityToMove) == 0) {
                        if ((a1->flags & OBJECT_IN_RIGHT_HAND) != 0) {
                            a3->fid = art_id(FID_TYPE(a3->fid), a3->fid & 0xFFF, FID_ANIM_TYPE(a3->fid), 0, a3->rotation + 1);
                        }

                        a3->flags &= ~OBJECT_EQUIPPED;

                        rc = 2;
                    } else {
                        // You cannot pick that up. You are at your maximum weight capacity.
                        messageListItem.num = 25;
                        if (messageListGetItem(&inventry_message_file, &messageListItem)) {
                            display_print(messageListItem.text);
                        }
                    }
                }
            }
        }
    }

    inven_set_mouse(INVENTORY_WINDOW_CURSOR_HAND);

    return rc;
}

// 0x474B2C
static int barter_compute_value(Object* a1, Object* a2)
{
    if (dialog_target_is_party) {
        return objectGetInventoryWeight(btable);
    }

    int cost = objectGetCost(btable);
    int caps = itemGetTotalCaps(btable);
    int v14 = cost - caps;

    double bonus = 0.0;
    if (a1 == gDude) {
        if (perkHasRank(gDude, PERK_MASTER_TRADER)) {
            bonus = 25.0;
        }
    }

    int partyBarter = partyGetBestSkillValue(SKILL_BARTER);
    int npcBarter = skillGetValue(a2, SKILL_BARTER);

    // TODO: Check in debugger, complex math, probably uses floats, not doubles.
    double v1 = (barter_mod + 100.0 - bonus) * 0.01;
    double v2 = (160.0 + npcBarter) / (160.0 + partyBarter) * (v14 * 2.0);
    if (v1 < 0) {
        // TODO: Probably 0.01 as float.
        v1 = 0.0099999998;
    }

    int rounded = (int)(v1 * v2 + caps);
    return rounded;
}

// 0x474C50
static int barter_attempt_transaction(Object* a1, Object* a2, Object* a3, Object* a4)
{
    MessageListItem messageListItem;

    int v8 = critterGetStat(a1, STAT_CARRY_WEIGHT) - objectGetInventoryWeight(a1);
    if (objectGetInventoryWeight(a4) > v8) {
        // Sorry, you cannot carry that much.
        messageListItem.num = 31;
        if (messageListGetItem(&inventry_message_file, &messageListItem)) {
            gdialogDisplayMsg(messageListItem.text);
        }
        return -1;
    }

    if (dialog_target_is_party) {
        int v10 = critterGetStat(a3, STAT_CARRY_WEIGHT) - objectGetInventoryWeight(a3);
        if (objectGetInventoryWeight(a2) > v10) {
            // Sorry, that's too much to carry.
            messageListItem.num = 32;
            if (messageListGetItem(&inventry_message_file, &messageListItem)) {
                gdialogDisplayMsg(messageListItem.text);
            }
            return -1;
        }
    } else {
        bool v11 = false;
        if (a2->data.inventory.length == 0) {
            v11 = true;
        } else {
            if (_item_queued(a2)) {
                if (a2->pid != PROTO_ID_GEIGER_COUNTER_I || miscItemTurnOff(a2) == -1) {
                    v11 = true;
                }
            }
        }

        if (!v11) {
            int cost = objectGetCost(a2);
            if (barter_compute_value(a1, a3) > cost) {
                v11 = true;
            }
        }

        if (v11) {
            // No, your offer is not good enough.
            messageListItem.num = 28;
            if (messageListGetItem(&inventry_message_file, &messageListItem)) {
                gdialogDisplayMsg(messageListItem.text);
            }
            return -1;
        }
    }

    _item_move_all(a4, a1);
    _item_move_all(a2, a3);
    return 0;
}

// 0x474DAC
static void barter_move_inventory(Object* a1, int quantity, int a3, int a4, Object* a5, Object* a6, bool a7)
{
    Rect rect;
    if (a7) {
        rect.left = 23;
        rect.top = 48 * a3 + 34;
    } else {
        rect.left = 395;
        rect.top = 48 * a3 + 31;
    }

    if (quantity > 1) {
        if (a7) {
            display_inventory(a4, a3, INVENTORY_WINDOW_TYPE_TRADE);
        } else {
            display_target_inventory(a4, a3, target_pud, INVENTORY_WINDOW_TYPE_TRADE);
        }
    } else {
        unsigned char* dest = windowGetBuffer(i_wid);
        unsigned char* src = windowGetBuffer(barter_back_win);

        int pitch = _scr_size.right - _scr_size.left + 1;
        blitBufferToBuffer(src + pitch * rect.top + rect.left + 80, INVENTORY_SLOT_WIDTH, INVENTORY_SLOT_HEIGHT, pitch, dest + 480 * rect.top + rect.left, 480);

        rect.right = rect.left + INVENTORY_SLOT_WIDTH - 1;
        rect.bottom = rect.top + INVENTORY_SLOT_HEIGHT - 1;
        win_draw_rect(i_wid, &rect);
    }

    CacheEntry* inventoryFrmHandle;
    int inventoryFid = itemGetInventoryFid(a1);
    Art* inventoryFrm = art_ptr_lock(inventoryFid, &inventoryFrmHandle);
    if (inventoryFrm != NULL) {
        int width = art_frame_width(inventoryFrm, 0, 0);
        int height = art_frame_length(inventoryFrm, 0, 0);
        unsigned char* data = art_frame_data(inventoryFrm, 0, 0);
        mouse_set_shape(data, width, height, width, width / 2, height / 2, 0);
        gsound_play_sfx_file("ipickup1");
    }

    do {
        _get_input();
    } while ((mouse_get_buttons() & MOUSE_EVENT_LEFT_BUTTON_REPEAT) != 0);

    if (inventoryFrm != NULL) {
        art_ptr_unlock(inventoryFrmHandle);
        gsound_play_sfx_file("iputdown");
    }

    MessageListItem messageListItem;

    if (a7) {
        if (mouse_click_in(INVENTORY_TRADE_LEFT_SCROLLER_TRACKING_ABS_X, INVENTORY_TRADE_LEFT_SCROLLER_TRACKING_ABS_Y, INVENTORY_TRADE_LEFT_SCROLLER_TRACKING_ABS_MAX_X, INVENTORY_SLOT_HEIGHT * inven_cur_disp + INVENTORY_TRADE_LEFT_SCROLLER_TRACKING_ABS_Y)) {
            int quantityToMove = quantity > 1 ? do_move_timer(INVENTORY_WINDOW_TYPE_MOVE_ITEMS, a1, quantity) : 1;
            if (quantityToMove != -1) {
                if (_item_move_force(inven_dude, a6, a1, quantityToMove) == -1) {
                    // There is no space left for that item.
                    messageListItem.num = 26;
                    if (messageListGetItem(&inventry_message_file, &messageListItem)) {
                        display_print(messageListItem.text);
                    }
                }
            }
        }
    } else {
        if (mouse_click_in(INVENTORY_TRADE_INNER_RIGHT_SCROLLER_TRACKING_ABS_X, INVENTORY_TRADE_INNER_RIGHT_SCROLLER_TRACKING_ABS_Y, INVENTORY_TRADE_INNER_RIGHT_SCROLLER_TRACKING_ABS_MAX_X, INVENTORY_SLOT_HEIGHT * inven_cur_disp + INVENTORY_TRADE_INNER_RIGHT_SCROLLER_TRACKING_ABS_Y)) {
            int quantityToMove = quantity > 1 ? do_move_timer(INVENTORY_WINDOW_TYPE_MOVE_ITEMS, a1, quantity) : 1;
            if (quantityToMove != -1) {
                if (_item_move_force(a5, a6, a1, quantityToMove) == -1) {
                    // You cannot pick that up. You are at your maximum weight capacity.
                    messageListItem.num = 25;
                    if (messageListGetItem(&inventry_message_file, &messageListItem)) {
                        display_print(messageListItem.text);
                    }
                }
            }
        }
    }

    inven_set_mouse(INVENTORY_WINDOW_CURSOR_HAND);
}

// 0x475070
static void barter_move_from_table_inventory(Object* a1, int quantity, int a3, Object* a4, Object* a5, bool a6)
{
    Rect rect;
    if (a6) {
        rect.left = 169;
        rect.top = 48 * a3 + 24;
    } else {
        rect.left = 254;
        rect.top = 48 * a3 + 24;
    }

    if (quantity > 1) {
        if (a6) {
            display_table_inventories(barter_back_win, a5, NULL, a3);
        } else {
            display_table_inventories(barter_back_win, NULL, a5, a3);
        }
    } else {
        unsigned char* dest = windowGetBuffer(i_wid);
        unsigned char* src = windowGetBuffer(barter_back_win);

        int pitch = _scr_size.right - _scr_size.left + 1;
        blitBufferToBuffer(src + pitch * rect.top + rect.left + 80, INVENTORY_SLOT_WIDTH, INVENTORY_SLOT_HEIGHT, pitch, dest + 480 * rect.top + rect.left, 480);

        rect.right = rect.left + INVENTORY_SLOT_WIDTH - 1;
        rect.bottom = rect.top + INVENTORY_SLOT_HEIGHT - 1;
        win_draw_rect(i_wid, &rect);
    }

    CacheEntry* inventoryFrmHandle;
    int inventoryFid = itemGetInventoryFid(a1);
    Art* inventoryFrm = art_ptr_lock(inventoryFid, &inventoryFrmHandle);
    if (inventoryFrm != NULL) {
        int width = art_frame_width(inventoryFrm, 0, 0);
        int height = art_frame_length(inventoryFrm, 0, 0);
        unsigned char* data = art_frame_data(inventoryFrm, 0, 0);
        mouse_set_shape(data, width, height, width, width / 2, height / 2, 0);
        gsound_play_sfx_file("ipickup1");
    }

    do {
        _get_input();
    } while ((mouse_get_buttons() & MOUSE_EVENT_LEFT_BUTTON_REPEAT) != 0);

    if (inventoryFrm != NULL) {
        art_ptr_unlock(inventoryFrmHandle);
        gsound_play_sfx_file("iputdown");
    }

    MessageListItem messageListItem;

    if (a6) {
        if (mouse_click_in(INVENTORY_TRADE_INNER_LEFT_SCROLLER_TRACKING_ABS_X, INVENTORY_TRADE_INNER_LEFT_SCROLLER_TRACKING_ABS_Y, INVENTORY_TRADE_INNER_LEFT_SCROLLER_TRACKING_ABS_MAX_X, INVENTORY_SLOT_HEIGHT * inven_cur_disp + INVENTORY_TRADE_INNER_LEFT_SCROLLER_TRACKING_ABS_Y)) {
            int quantityToMove = quantity > 1 ? do_move_timer(INVENTORY_WINDOW_TYPE_MOVE_ITEMS, a1, quantity) : 1;
            if (quantityToMove != -1) {
                if (_item_move_force(a5, inven_dude, a1, quantityToMove) == -1) {
                    // There is no space left for that item.
                    messageListItem.num = 26;
                    if (messageListGetItem(&inventry_message_file, &messageListItem)) {
                        display_print(messageListItem.text);
                    }
                }
            }
        }
    } else {
        if (mouse_click_in(INVENTORY_TRADE_RIGHT_SCROLLER_TRACKING_ABS_X, INVENTORY_TRADE_RIGHT_SCROLLER_TRACKING_ABS_Y, INVENTORY_TRADE_RIGHT_SCROLLER_TRACKING_ABS_MAX_X, INVENTORY_SLOT_HEIGHT * inven_cur_disp + INVENTORY_TRADE_RIGHT_SCROLLER_TRACKING_ABS_Y)) {
            int quantityToMove = quantity > 1 ? do_move_timer(INVENTORY_WINDOW_TYPE_MOVE_ITEMS, a1, quantity) : 1;
            if (quantityToMove != -1) {
                if (_item_move_force(a5, a4, a1, quantityToMove) == -1) {
                    // You cannot pick that up. You are at your maximum weight capacity.
                    messageListItem.num = 25;
                    if (messageListGetItem(&inventry_message_file, &messageListItem)) {
                        display_print(messageListItem.text);
                    }
                }
            }
        }
    }

    inven_set_mouse(INVENTORY_WINDOW_CURSOR_HAND);
}

// 0x475334
static void display_table_inventories(int win, Object* a2, Object* a3, int a4)
{
    unsigned char* windowBuffer = windowGetBuffer(i_wid);

    int oldFont = fontGetCurrent();
    fontSetCurrent(101);

    char formattedText[80];
    int v45 = fontGetLineHeight() + 48 * inven_cur_disp;

    if (a2 != NULL) {
        unsigned char* src = windowGetBuffer(win);
        blitBufferToBuffer(src + (_scr_size.right - _scr_size.left + 1) * 20 + 249, 64, v45 + 1, _scr_size.right - _scr_size.left + 1, windowBuffer + 480 * 20 + 169, 480);

        unsigned char* dest = windowBuffer + 480 * 24 + 169;
        Inventory* inventory = &(a2->data.inventory);
        for (int index = 0; index < inven_cur_disp && index + ptable_offset < inventory->length; index++) {
            InventoryItem* inventoryItem = &(inventory->items[inventory->length - (index + ptable_offset + 1)]);
            int inventoryFid = itemGetInventoryFid(inventoryItem->item);
            scale_art(inventoryFid, dest, 56, 40, 480);
            display_inventory_info(inventoryItem->item, inventoryItem->quantity, dest, 480, index == a4);

            dest += 480 * 48;
        }

        if (dialog_target_is_party) {
            MessageListItem messageListItem;
            messageListItem.num = 30;

            if (messageListGetItem(&inventry_message_file, &messageListItem)) {
                int weight = objectGetInventoryWeight(a2);
                sprintf(formattedText, "%s %d", messageListItem.text, weight);
            }
        } else {
            int cost = objectGetCost(a2);
            sprintf(formattedText, "$%d", cost);
        }

        fontDrawText(windowBuffer + 480 * (48 * inven_cur_disp + 24) + 169, formattedText, 80, 480, colorTable[32767]);

        Rect rect;
        rect.left = 169;
        rect.top = 24;
        rect.right = 223;
        rect.bottom = rect.top + v45;
        win_draw_rect(i_wid, &rect);
    }

    if (a3 != NULL) {
        unsigned char* src = windowGetBuffer(win);
        blitBufferToBuffer(src + (_scr_size.right - _scr_size.left + 1) * 20 + 334, 64, v45 + 1, _scr_size.right - _scr_size.left + 1, windowBuffer + 480 * 20 + 254, 480);

        unsigned char* dest = windowBuffer + 480 * 24 + 254;
        Inventory* inventory = &(a3->data.inventory);
        for (int index = 0; index < inven_cur_disp && index + btable_offset < inventory->length; index++) {
            InventoryItem* inventoryItem = &(inventory->items[inventory->length - (index + btable_offset + 1)]);
            int inventoryFid = itemGetInventoryFid(inventoryItem->item);
            scale_art(inventoryFid, dest, 56, 40, 480);
            display_inventory_info(inventoryItem->item, inventoryItem->quantity, dest, 480, index == a4);

            dest += 480 * 48;
        }

        if (dialog_target_is_party) {
            MessageListItem messageListItem;
            messageListItem.num = 30;

            if (messageListGetItem(&inventry_message_file, &messageListItem)) {
                int weight = barter_compute_value(gDude, target_stack[0]);
                sprintf(formattedText, "%s %d", messageListItem.text, weight);
            }
        } else {
            int cost = barter_compute_value(gDude, target_stack[0]);
            sprintf(formattedText, "$%d", cost);
        }

        fontDrawText(windowBuffer + 480 * (48 * inven_cur_disp + 24) + 254, formattedText, 80, 480, colorTable[32767]);

        Rect rect;
        rect.left = 254;
        rect.top = 24;
        rect.right = 318;
        rect.bottom = rect.top + v45;
        win_draw_rect(i_wid, &rect);
    }

    fontSetCurrent(oldFont);
}

// 0x4757F0
void barter_inventory(int win, Object* a2, Object* a3, Object* a4, int a5)
{
    barter_mod = a5;

    if (inven_init() == -1) {
        return;
    }

    Object* armor = inven_worn(a2);
    if (armor != NULL) {
        itemRemove(a2, armor, 1);
    }

    Object* item1 = NULL;
    Object* item2 = inven_right_hand(a2);
    if (item2 != NULL) {
        itemRemove(a2, item2, 1);
    } else {
        if (!dialog_target_is_party) {
            item1 = inven_find_type(a2, ITEM_TYPE_WEAPON, NULL);
            if (item1 != NULL) {
                itemRemove(a2, item1, 1);
            }
        }
    }

    Object* a1a = NULL;
    if (objectCreateWithFidPid(&a1a, 0, 467) == -1) {
        return;
    }

    pud = &(inven_dude->data.inventory);
    btable = a4;
    ptable = a3;

    ptable_offset = 0;
    btable_offset = 0;

    ptable_pud = &(a3->data.inventory);
    btable_pud = &(a4->data.inventory);

    barter_back_win = win;
    target_curr_stack = 0;
    target_pud = &(a2->data.inventory);

    target_stack[0] = a2;
    target_stack_offset[0] = 0;

    bool isoWasEnabled = setup_inventory(INVENTORY_WINDOW_TYPE_TRADE);
    display_target_inventory(target_stack_offset[target_curr_stack], -1, target_pud, INVENTORY_WINDOW_TYPE_TRADE);
    display_inventory(stack_offset[0], -1, INVENTORY_WINDOW_TYPE_TRADE);
    display_body(a2->fid, INVENTORY_WINDOW_TYPE_TRADE);
    win_draw(barter_back_win);
    display_table_inventories(win, a3, a4, -1);

    inven_set_mouse(INVENTORY_WINDOW_CURSOR_HAND);

    int modifier;
    int npcReactionValue = reactionGetValue(a2);
    int npcReactionType = reactionTranslateValue(npcReactionValue);
    switch (npcReactionType) {
    case NPC_REACTION_BAD:
        modifier = 25;
        break;
    case NPC_REACTION_NEUTRAL:
        modifier = 0;
        break;
    case NPC_REACTION_GOOD:
        modifier = -15;
        break;
    default:
        assert(false && "Should be unreachable");
    }

    int keyCode = -1;
    for (;;) {
        if (keyCode == KEY_ESCAPE || game_user_wants_to_quit != 0) {
            break;
        }

        keyCode = _get_input();
        if (keyCode == KEY_CTRL_Q || keyCode == KEY_CTRL_X || keyCode == KEY_F10) {
            game_quit_with_confirm();
        }

        if (game_user_wants_to_quit != 0) {
            break;
        }

        barter_mod = a5 + modifier;

        if (keyCode == KEY_LOWERCASE_T || modifier <= -30) {
            _item_move_all(a4, a2);
            _item_move_all(a3, gDude);
            barter_end_to_talk_to();
            break;
        } else if (keyCode == KEY_LOWERCASE_M) {
            if (a3->data.inventory.length != 0 || btable->data.inventory.length != 0) {
                if (barter_attempt_transaction(inven_dude, a3, a2, a4) == 0) {
                    display_target_inventory(target_stack_offset[target_curr_stack], -1, target_pud, INVENTORY_WINDOW_TYPE_TRADE);
                    display_inventory(stack_offset[curr_stack], -1, INVENTORY_WINDOW_TYPE_TRADE);
                    display_table_inventories(win, a3, a4, -1);

                    // Ok, that's a good trade.
                    MessageListItem messageListItem;
                    messageListItem.num = 27;
                    if (!dialog_target_is_party) {
                        if (messageListGetItem(&inventry_message_file, &messageListItem)) {
                            gdialogDisplayMsg(messageListItem.text);
                        }
                    }
                }
            }
        } else if (keyCode == KEY_ARROW_UP) {
            if (stack_offset[curr_stack] > 0) {
                stack_offset[curr_stack] -= 1;
                display_inventory(stack_offset[curr_stack], -1, INVENTORY_WINDOW_TYPE_TRADE);
            }
        } else if (keyCode == KEY_PAGE_UP) {
            if (ptable_offset > 0) {
                ptable_offset -= 1;
                display_table_inventories(win, a3, a4, -1);
            }
        } else if (keyCode == KEY_ARROW_DOWN) {
            if (stack_offset[curr_stack] + inven_cur_disp < pud->length) {
                stack_offset[curr_stack] += 1;
                display_inventory(stack_offset[curr_stack], -1, INVENTORY_WINDOW_TYPE_TRADE);
            }
        } else if (keyCode == KEY_PAGE_DOWN) {
            if (ptable_offset + inven_cur_disp < ptable_pud->length) {
                ptable_offset += 1;
                display_table_inventories(win, a3, a4, -1);
            }
        } else if (keyCode == KEY_CTRL_PAGE_DOWN) {
            if (btable_offset + inven_cur_disp < btable_pud->length) {
                btable_offset++;
                display_table_inventories(win, a3, a4, -1);
            }
        } else if (keyCode == KEY_CTRL_PAGE_UP) {
            if (btable_offset > 0) {
                btable_offset -= 1;
                display_table_inventories(win, a3, a4, -1);
            }
        } else if (keyCode == KEY_CTRL_ARROW_UP) {
            if (target_stack_offset[target_curr_stack] > 0) {
                target_stack_offset[target_curr_stack] -= 1;
                display_target_inventory(target_stack_offset[target_curr_stack], -1, target_pud, INVENTORY_WINDOW_TYPE_TRADE);
                win_draw(i_wid);
            }
        } else if (keyCode == KEY_CTRL_ARROW_DOWN) {
            if (target_stack_offset[target_curr_stack] + inven_cur_disp < target_pud->length) {
                target_stack_offset[target_curr_stack] += 1;
                display_target_inventory(target_stack_offset[target_curr_stack], -1, target_pud, INVENTORY_WINDOW_TYPE_TRADE);
                win_draw(i_wid);
            }
        } else if (keyCode >= 2500 && keyCode <= 2501) {
            container_exit(keyCode, INVENTORY_WINDOW_TYPE_TRADE);
        } else {
            if ((mouse_get_buttons() & MOUSE_EVENT_RIGHT_BUTTON_DOWN) != 0) {
                if (immode == INVENTORY_WINDOW_CURSOR_HAND) {
                    inven_set_mouse(INVENTORY_WINDOW_CURSOR_ARROW);
                } else {
                    inven_set_mouse(INVENTORY_WINDOW_CURSOR_HAND);
                }
            } else if ((mouse_get_buttons() & MOUSE_EVENT_LEFT_BUTTON_DOWN) != 0) {
                if (keyCode >= 1000 && keyCode <= 1000 + inven_cur_disp) {
                    if (immode == INVENTORY_WINDOW_CURSOR_ARROW) {
                        inven_action_cursor(keyCode, INVENTORY_WINDOW_TYPE_TRADE);
                        display_table_inventories(win, a3, NULL, -1);
                    } else {
                        int v30 = keyCode - 1000;
                        if (v30 + stack_offset[curr_stack] < pud->length) {
                            int v31 = stack_offset[curr_stack];
                            InventoryItem* inventoryItem = &(pud->items[pud->length - (v30 + v31 + 1)]);
                            barter_move_inventory(inventoryItem->item, inventoryItem->quantity, v30, v31, a2, a3, true);
                            display_target_inventory(target_stack_offset[target_curr_stack], -1, target_pud, INVENTORY_WINDOW_TYPE_TRADE);
                            display_inventory(stack_offset[curr_stack], -1, INVENTORY_WINDOW_TYPE_TRADE);
                            display_table_inventories(win, a3, NULL, -1);
                        }
                    }

                    keyCode = -1;
                } else if (keyCode >= 2000 && keyCode <= 2000 + inven_cur_disp) {
                    if (immode == INVENTORY_WINDOW_CURSOR_ARROW) {
                        inven_action_cursor(keyCode, INVENTORY_WINDOW_TYPE_TRADE);
                        display_table_inventories(win, NULL, a4, -1);
                    } else {
                        int v35 = keyCode - 2000;
                        if (v35 + target_stack_offset[target_curr_stack] < target_pud->length) {
                            int v36 = target_stack_offset[target_curr_stack];
                            InventoryItem* inventoryItem = &(target_pud->items[target_pud->length - (v35 + v36 + 1)]);
                            barter_move_inventory(inventoryItem->item, inventoryItem->quantity, v35, v36, a2, a4, false);
                            display_target_inventory(target_stack_offset[target_curr_stack], -1, target_pud, INVENTORY_WINDOW_TYPE_TRADE);
                            display_inventory(stack_offset[curr_stack], -1, INVENTORY_WINDOW_TYPE_TRADE);
                            display_table_inventories(win, NULL, a4, -1);
                        }
                    }

                    keyCode = -1;
                } else if (keyCode >= 2300 && keyCode <= 2300 + inven_cur_disp) {
                    if (immode == INVENTORY_WINDOW_CURSOR_ARROW) {
                        inven_action_cursor(keyCode, INVENTORY_WINDOW_TYPE_TRADE);
                        display_table_inventories(win, a3, NULL, -1);
                    } else {
                        int v41 = keyCode - 2300;
                        if (v41 < ptable_pud->length) {
                            InventoryItem* inventoryItem = &(ptable_pud->items[ptable_pud->length - (v41 + ptable_offset + 1)]);
                            barter_move_from_table_inventory(inventoryItem->item, inventoryItem->quantity, v41, a2, a3, true);
                            display_target_inventory(target_stack_offset[target_curr_stack], -1, target_pud, INVENTORY_WINDOW_TYPE_TRADE);
                            display_inventory(stack_offset[curr_stack], -1, INVENTORY_WINDOW_TYPE_TRADE);
                            display_table_inventories(win, a3, NULL, -1);
                        }
                    }

                    keyCode = -1;
                } else if (keyCode >= 2400 && keyCode <= 2400 + inven_cur_disp) {
                    if (immode == INVENTORY_WINDOW_CURSOR_ARROW) {
                        inven_action_cursor(keyCode, INVENTORY_WINDOW_TYPE_TRADE);
                        display_table_inventories(win, NULL, a4, -1);
                    } else {
                        int v45 = keyCode - 2400;
                        if (v45 < btable_pud->length) {
                            InventoryItem* inventoryItem = &(btable_pud->items[btable_pud->length - (v45 + btable_offset + 1)]);
                            barter_move_from_table_inventory(inventoryItem->item, inventoryItem->quantity, v45, a2, a4, false);
                            display_target_inventory(target_stack_offset[target_curr_stack], -1, target_pud, INVENTORY_WINDOW_TYPE_TRADE);
                            display_inventory(stack_offset[curr_stack], -1, INVENTORY_WINDOW_TYPE_TRADE);
                            display_table_inventories(win, NULL, a4, -1);
                        }
                    }

                    keyCode = -1;
                }
            }
        }
    }

    _item_move_all(a1a, a2);
    objectDestroy(a1a, NULL);

    if (armor != NULL) {
        armor->flags |= OBJECT_WORN;
        itemAdd(a2, armor, 1);
    }

    if (item2 != NULL) {
        item2->flags |= OBJECT_IN_RIGHT_HAND;
        itemAdd(a2, item2, 1);
    }

    if (item1 != NULL) {
        itemAdd(a2, item1, 1);
    }

    exit_inventory(isoWasEnabled);

    // NOTE: Uninline.
    inven_exit();
}

// 0x47620C
void container_enter(int keyCode, int inventoryWindowType)
{
    if (keyCode >= 2000) {
        int index = target_pud->length - (target_stack_offset[target_curr_stack] + keyCode - 2000 + 1);
        if (index < target_pud->length && target_curr_stack < 9) {
            InventoryItem* inventoryItem = &(target_pud->items[index]);
            Object* item = inventoryItem->item;
            if (itemGetType(item) == ITEM_TYPE_CONTAINER) {
                target_curr_stack += 1;
                target_stack[target_curr_stack] = item;
                target_stack_offset[target_curr_stack] = 0;

                target_pud = &(item->data.inventory);

                display_body(item->fid, inventoryWindowType);
                display_target_inventory(target_stack_offset[target_curr_stack], -1, target_pud, inventoryWindowType);
                win_draw(i_wid);
            }
        }
    } else {
        int index = pud->length - (stack_offset[curr_stack] + keyCode - 1000 + 1);
        if (index < pud->length && curr_stack < 9) {
            InventoryItem* inventoryItem = &(pud->items[index]);
            Object* item = inventoryItem->item;
            if (itemGetType(item) == ITEM_TYPE_CONTAINER) {
                curr_stack += 1;

                stack[curr_stack] = item;
                stack_offset[curr_stack] = 0;

                inven_dude = stack[curr_stack];
                pud = &(item->data.inventory);

                adjust_fid();
                display_body(-1, inventoryWindowType);
                display_inventory(stack_offset[curr_stack], -1, inventoryWindowType);
            }
        }
    }
}

// 0x476394
void container_exit(int keyCode, int inventoryWindowType)
{
    if (keyCode == 2500) {
        if (curr_stack > 0) {
            curr_stack -= 1;
            inven_dude = stack[curr_stack];
            pud = &inven_dude->data.inventory;
            adjust_fid();
            display_body(-1, inventoryWindowType);
            display_inventory(stack_offset[curr_stack], -1, inventoryWindowType);
        }
    } else if (keyCode == 2501) {
        if (target_curr_stack > 0) {
            target_curr_stack -= 1;
            Object* v5 = target_stack[target_curr_stack];
            target_pud = &(v5->data.inventory);
            display_body(v5->fid, inventoryWindowType);
            display_target_inventory(target_stack_offset[target_curr_stack], -1, target_pud, inventoryWindowType);
            win_draw(i_wid);
        }
    }
}

// 0x476464
int drop_into_container(Object* a1, Object* a2, int a3, Object** a4, int quantity)
{
    int quantityToMove;
    if (quantity > 1) {
        quantityToMove = do_move_timer(INVENTORY_WINDOW_TYPE_MOVE_ITEMS, a2, quantity);
    } else {
        quantityToMove = 1;
    }

    if (quantityToMove == -1) {
        return -1;
    }

    if (a3 != -1) {
        if (itemRemove(inven_dude, a2, quantityToMove) == -1) {
            return -1;
        }
    }

    int rc = itemAttemptAdd(a1, a2, quantityToMove);
    if (rc != 0) {
        if (a3 != -1) {
            itemAttemptAdd(inven_dude, a2, quantityToMove);
        }
    } else {
        if (a4 != NULL) {
            if (a4 == &i_worn) {
                adjust_ac(stack[0], i_worn, NULL);
            }
            *a4 = NULL;
        }
    }

    return rc;
}

// 0x47650C
int drop_ammo_into_weapon(Object* weapon, Object* ammo, Object** a3, int quantity, int keyCode)
{
    if (itemGetType(weapon) != ITEM_TYPE_WEAPON) {
        return -1;
    }

    if (itemGetType(ammo) != ITEM_TYPE_AMMO) {
        return -1;
    }

    if (!weaponCanBeReloadedWith(weapon, ammo)) {
        return -1;
    }

    int quantityToMove;
    if (quantity > 1) {
        quantityToMove = do_move_timer(INVENTORY_WINDOW_TYPE_MOVE_ITEMS, ammo, quantity);
    } else {
        quantityToMove = 1;
    }

    if (quantityToMove == -1) {
        return -1;
    }

    Object* v14 = ammo;
    bool v17 = false;
    int rc = itemRemove(inven_dude, weapon, 1);
    for (int index = 0; index < quantityToMove; index++) {
        int v11 = _item_w_reload(weapon, v14);
        if (v11 == 0) {
            if (a3 != NULL) {
                *a3 = NULL;
            }

            _obj_destroy(v14);

            v17 = true;
            if (inven_from_button(keyCode, &v14, NULL, NULL) == 0) {
                break;
            }
        }
        if (v11 != -1) {
            v17 = true;
        }
        if (v11 != 0) {
            break;
        }
    }

    if (rc != -1) {
        itemAdd(inven_dude, weapon, 1);
    }

    if (!v17) {
        return -1;
    }

    const char* sfx = gsnd_build_weapon_sfx_name(WEAPON_SOUND_EFFECT_READY, weapon, HIT_MODE_RIGHT_WEAPON_PRIMARY, NULL);
    gsound_play_sfx_file(sfx);

    return 0;
}

// 0x47664C
void draw_amount(int value, int inventoryWindowType)
{
    // BIGNUM.frm
    CacheEntry* handle;
    int fid = art_id(OBJ_TYPE_INTERFACE, 170, 0, 0, 0);
    unsigned char* data = art_ptr_lock_data(fid, 0, 0, &handle);
    if (data == NULL) {
        return;
    }

    Rect rect;

    int windowWidth = windowGetWidth(mt_wid);
    unsigned char* windowBuffer = windowGetBuffer(mt_wid);

    if (inventoryWindowType == INVENTORY_WINDOW_TYPE_MOVE_ITEMS) {
        rect.left = 125;
        rect.top = 45;
        rect.right = 195;
        rect.bottom = 69;

        int ranks[5];
        ranks[4] = value % 10;
        ranks[3] = value / 10 % 10;
        ranks[2] = value / 100 % 10;
        ranks[1] = value / 1000 % 10;
        ranks[0] = value / 10000 % 10;

        windowBuffer += rect.top * windowWidth + rect.left;

        for (int index = 0; index < 5; index++) {
            unsigned char* src = data + 14 * ranks[index];
            blitBufferToBuffer(src, 14, 24, 336, windowBuffer, windowWidth);
            windowBuffer += 14;
        }
    } else {
        rect.left = 133;
        rect.top = 64;
        rect.right = 189;
        rect.bottom = 88;

        windowBuffer += windowWidth * rect.top + rect.left;
        blitBufferToBuffer(data + 14 * (value / 60), 14, 24, 336, windowBuffer, windowWidth);
        blitBufferToBuffer(data + 14 * (value % 60 / 10), 14, 24, 336, windowBuffer + 14 * 2, windowWidth);
        blitBufferToBuffer(data + 14 * (value % 10), 14, 24, 336, windowBuffer + 14 * 3, windowWidth);
    }

    art_ptr_unlock(handle);
    win_draw_rect(mt_wid, &rect);
}

// 0x47688C
static int do_move_timer(int inventoryWindowType, Object* item, int max)
{
    setup_move_timer_win(inventoryWindowType, item);

    int value;
    int min;
    if (inventoryWindowType == INVENTORY_WINDOW_TYPE_MOVE_ITEMS) {
        value = 1;
        if (max > 99999) {
            max = 99999;
        }
        min = 1;
    } else {
        value = 60;
        min = 10;
    }

    draw_amount(value, inventoryWindowType);

    bool v5 = false;
    for (;;) {
        int keyCode = _get_input();
        if (keyCode == KEY_ESCAPE) {
            exit_move_timer_win(inventoryWindowType);
            return -1;
        }

        if (keyCode == KEY_RETURN) {
            if (value >= min && value <= max) {
                if (inventoryWindowType != INVENTORY_WINDOW_TYPE_SET_TIMER || value % 10 == 0) {
                    gsound_play_sfx_file("ib1p1xx1");
                    break;
                }
            }

            gsound_play_sfx_file("iisxxxx1");
        } else if (keyCode == 5000) {
            v5 = false;
            value = max;
            draw_amount(value, inventoryWindowType);
        } else if (keyCode == 6000) {
            v5 = false;
            if (value < max) {
                if (inventoryWindowType == INVENTORY_WINDOW_TYPE_MOVE_ITEMS) {
                    if ((mouse_get_buttons() & MOUSE_EVENT_LEFT_BUTTON_REPEAT) != 0) {
                        _get_time();

                        unsigned int delay = 100;
                        while ((mouse_get_buttons() & MOUSE_EVENT_LEFT_BUTTON_REPEAT) != 0) {
                            if (value < max) {
                                value++;
                            }

                            draw_amount(value, inventoryWindowType);
                            _get_input();

                            if (delay > 1) {
                                delay--;
                                coreDelayProcessingEvents(delay);
                            }
                        }
                    } else {
                        if (value < max) {
                            value++;
                        }
                    }
                } else {
                    value += 10;
                }

                draw_amount(value, inventoryWindowType);
                continue;
            }
        } else if (keyCode == 7000) {
            v5 = false;
            if (value > min) {
                if (inventoryWindowType == INVENTORY_WINDOW_TYPE_MOVE_ITEMS) {
                    if ((mouse_get_buttons() & MOUSE_EVENT_LEFT_BUTTON_REPEAT) != 0) {
                        _get_time();

                        unsigned int delay = 100;
                        while ((mouse_get_buttons() & MOUSE_EVENT_LEFT_BUTTON_REPEAT) != 0) {
                            if (value > min) {
                                value--;
                            }

                            draw_amount(value, inventoryWindowType);
                            _get_input();

                            if (delay > 1) {
                                delay--;
                                coreDelayProcessingEvents(delay);
                            }
                        }
                    } else {
                        if (value > min) {
                            value--;
                        }
                    }
                } else {
                    value -= 10;
                }

                draw_amount(value, inventoryWindowType);
                continue;
            }
        }

        if (inventoryWindowType == INVENTORY_WINDOW_TYPE_MOVE_ITEMS) {
            if (keyCode >= KEY_0 && keyCode <= KEY_9) {
                int number = keyCode - KEY_0;
                if (!v5) {
                    value = 0;
                }

                value = 10 * value % 100000 + number;
                v5 = true;

                draw_amount(value, inventoryWindowType);
                continue;
            } else if (keyCode == KEY_BACKSPACE) {
                if (!v5) {
                    value = 0;
                }

                value /= 10;
                v5 = true;

                draw_amount(value, inventoryWindowType);
                continue;
            }
        }
    }

    exit_move_timer_win(inventoryWindowType);

    return value;
}

// Creates move items/set timer interface.
//
// 0x476AB8
static int setup_move_timer_win(int inventoryWindowType, Object* item)
{
    const int oldFont = fontGetCurrent();
    fontSetCurrent(103);

    for (int index = 0; index < 8; index++) {
        mt_key[index] = NULL;
    }

    InventoryWindowDescription* windowDescription = &(iscr_data[inventoryWindowType]);

    int quantityWindowX = windowDescription->x;
    int quantityWindowY = windowDescription->y;
    mt_wid = windowCreate(quantityWindowX, quantityWindowY, windowDescription->width, windowDescription->height, 257, WINDOW_FLAG_0x10 | WINDOW_FLAG_0x04);
    unsigned char* windowBuffer = windowGetBuffer(mt_wid);

    CacheEntry* backgroundHandle;
    int backgroundFid = art_id(OBJ_TYPE_INTERFACE, windowDescription->field_0, 0, 0, 0);
    unsigned char* backgroundData = art_ptr_lock_data(backgroundFid, 0, 0, &backgroundHandle);
    if (backgroundData != NULL) {
        blitBufferToBuffer(backgroundData, windowDescription->width, windowDescription->height, windowDescription->width, windowBuffer, windowDescription->width);
        art_ptr_unlock(backgroundHandle);
    }

    MessageListItem messageListItem;
    if (inventoryWindowType == INVENTORY_WINDOW_TYPE_MOVE_ITEMS) {
        // MOVE ITEMS
        messageListItem.num = 21;
        if (messageListGetItem(&inventry_message_file, &messageListItem)) {
            int length = fontGetStringWidth(messageListItem.text);
            fontDrawText(windowBuffer + windowDescription->width * 9 + (windowDescription->width - length) / 2, messageListItem.text, 200, windowDescription->width, colorTable[21091]);
        }
    } else if (inventoryWindowType == INVENTORY_WINDOW_TYPE_SET_TIMER) {
        // SET TIMER
        messageListItem.num = 23;
        if (messageListGetItem(&inventry_message_file, &messageListItem)) {
            int length = fontGetStringWidth(messageListItem.text);
            fontDrawText(windowBuffer + windowDescription->width * 9 + (windowDescription->width - length) / 2, messageListItem.text, 200, windowDescription->width, colorTable[21091]);
        }

        // Timer overlay
        CacheEntry* overlayFrmHandle;
        int overlayFid = art_id(OBJ_TYPE_INTERFACE, 306, 0, 0, 0);
        unsigned char* overlayFrmData = art_ptr_lock_data(overlayFid, 0, 0, &overlayFrmHandle);
        if (overlayFrmData != NULL) {
            blitBufferToBuffer(overlayFrmData, 105, 81, 105, windowBuffer + 34 * windowDescription->width + 113, windowDescription->width);
            art_ptr_unlock(overlayFrmHandle);
        }
    }

    int inventoryFid = itemGetInventoryFid(item);
    scale_art(inventoryFid, windowBuffer + windowDescription->width * 46 + 16, INVENTORY_LARGE_SLOT_WIDTH, INVENTORY_LARGE_SLOT_HEIGHT, windowDescription->width);

    int x;
    int y;
    if (inventoryWindowType == INVENTORY_WINDOW_TYPE_MOVE_ITEMS) {
        x = 200;
        y = 46;
    } else {
        x = 194;
        y = 64;
    }

    int fid;
    unsigned char* buttonUpData;
    unsigned char* buttonDownData;
    int btn;

    // Plus button
    fid = art_id(OBJ_TYPE_INTERFACE, 193, 0, 0, 0);
    buttonUpData = art_ptr_lock_data(fid, 0, 0, &(mt_key[0]));

    fid = art_id(OBJ_TYPE_INTERFACE, 194, 0, 0, 0);
    buttonDownData = art_ptr_lock_data(fid, 0, 0, &(mt_key[1]));

    if (buttonUpData != NULL && buttonDownData != NULL) {
        btn = buttonCreate(mt_wid, x, y, 16, 12, -1, -1, 6000, -1, buttonUpData, buttonDownData, NULL, BUTTON_FLAG_TRANSPARENT);
        if (btn != -1) {
            buttonSetCallbacks(btn, gsound_red_butt_press, gsound_red_butt_release);
        }
    }

    // Minus button
    fid = art_id(OBJ_TYPE_INTERFACE, 191, 0, 0, 0);
    buttonUpData = art_ptr_lock_data(fid, 0, 0, &(mt_key[2]));

    fid = art_id(OBJ_TYPE_INTERFACE, 192, 0, 0, 0);
    buttonDownData = art_ptr_lock_data(fid, 0, 0, &(mt_key[3]));

    if (buttonUpData != NULL && buttonDownData != NULL) {
        btn = buttonCreate(mt_wid, x, y + 12, 17, 12, -1, -1, 7000, -1, buttonUpData, buttonDownData, NULL, BUTTON_FLAG_TRANSPARENT);
        if (btn != -1) {
            buttonSetCallbacks(btn, gsound_red_butt_press, gsound_red_butt_release);
        }
    }

    fid = art_id(OBJ_TYPE_INTERFACE, 8, 0, 0, 0);
    buttonUpData = art_ptr_lock_data(fid, 0, 0, &(mt_key[4]));

    fid = art_id(OBJ_TYPE_INTERFACE, 9, 0, 0, 0);
    buttonDownData = art_ptr_lock_data(fid, 0, 0, &(mt_key[5]));

    if (buttonUpData != NULL && buttonDownData != NULL) {
        // Done
        btn = buttonCreate(mt_wid, 98, 128, 15, 16, -1, -1, -1, KEY_RETURN, buttonUpData, buttonDownData, NULL, BUTTON_FLAG_TRANSPARENT);
        if (btn != -1) {
            buttonSetCallbacks(btn, gsound_red_butt_press, gsound_red_butt_release);
        }

        // Cancel
        btn = buttonCreate(mt_wid, 148, 128, 15, 16, -1, -1, -1, KEY_ESCAPE, buttonUpData, buttonDownData, NULL, BUTTON_FLAG_TRANSPARENT);
        if (btn != -1) {
            buttonSetCallbacks(btn, gsound_red_butt_press, gsound_red_butt_release);
        }
    }

    if (inventoryWindowType == INVENTORY_WINDOW_TYPE_MOVE_ITEMS) {
        fid = art_id(OBJ_TYPE_INTERFACE, 307, 0, 0, 0);
        buttonUpData = art_ptr_lock_data(fid, 0, 0, &(mt_key[6]));

        fid = art_id(OBJ_TYPE_INTERFACE, 308, 0, 0, 0);
        buttonDownData = art_ptr_lock_data(fid, 0, 0, &(mt_key[7]));

        if (buttonUpData != NULL && buttonDownData != NULL) {
            // ALL
            messageListItem.num = 22;
            if (messageListGetItem(&inventry_message_file, &messageListItem)) {
                int length = fontGetStringWidth(messageListItem.text);

                // TODO: Where is y? Is it hardcoded in to 376?
                fontDrawText(buttonUpData + (94 - length) / 2 + 376, messageListItem.text, 200, 94, colorTable[21091]);
                fontDrawText(buttonDownData + (94 - length) / 2 + 376, messageListItem.text, 200, 94, colorTable[18977]);

                btn = buttonCreate(mt_wid, 120, 80, 94, 33, -1, -1, -1, 5000, buttonUpData, buttonDownData, NULL, BUTTON_FLAG_TRANSPARENT);
                if (btn != -1) {
                    buttonSetCallbacks(btn, gsound_red_butt_press, gsound_red_butt_release);
                }
            }
        }
    }

    win_draw(mt_wid);
    inven_set_mouse(INVENTORY_WINDOW_CURSOR_ARROW);
    fontSetCurrent(oldFont);

    return 0;
}

// 0x477030
static int exit_move_timer_win(int inventoryWindowType)
{
    int count = inventoryWindowType == INVENTORY_WINDOW_TYPE_MOVE_ITEMS ? 8 : 6;

    for (int index = 0; index < count; index++) {
        art_ptr_unlock(mt_key[index]);
    }

    windowDestroy(mt_wid);

    return 0;
}

// 0x477074
int inven_set_timer(Object* a1)
{
    bool v1 = inven_is_initialized;

    if (!v1) {
        if (inven_init() == -1) {
            return -1;
        }
    }

    int seconds = do_move_timer(INVENTORY_WINDOW_TYPE_SET_TIMER, a1, 180);

    if (!v1) {
        // NOTE: Uninline.
        inven_exit();
    }

    return seconds;
}