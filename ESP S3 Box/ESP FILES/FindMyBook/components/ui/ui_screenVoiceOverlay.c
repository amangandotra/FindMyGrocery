#include "../ui.h"

/* =========================
   FORWARD DECLARATION
   ========================= */

void ui_screenVoiceOverlay_screen_init(void);

/* =========================
   OBJECTS
   ========================= */

lv_obj_t * ui_screenVoiceOverlay = NULL;
lv_obj_t * ui_voicePanel = NULL;
lv_obj_t * ui_voiceLabel = NULL;

/* =========================
   SHOW OVERLAY
   ========================= */

void voice_overlay_show(const char *text)
{
    /* Create screen if not exists */
    if(!ui_screenVoiceOverlay)
    {
        ui_screenVoiceOverlay_screen_init();
    }

    /* Update label text safely */
    if(ui_voiceLabel)
    {
        lv_label_set_text(ui_voiceLabel, text);
    }

    /* Smooth fade transition */
    _ui_screen_change(
        &ui_screenVoiceOverlay,
        LV_SCR_LOAD_ANIM_FADE_IN,   // ✅ correct enum
        250,
        0,
        &ui_screenVoiceOverlay_screen_init
    );
}

/* =========================
   HIDE OVERLAY
   ========================= */

void voice_overlay_hide(lv_obj_t **target_screen,
                        void (*init_func)(void))
{
    _ui_screen_change(
        target_screen,
        LV_SCR_LOAD_ANIM_FADE_OUT,   // smoother exit
        200,
        0,
        init_func
    );
}

/* =========================
   BUILD SCREEN
   ========================= */

void ui_screenVoiceOverlay_screen_init(void)
{
    ui_screenVoiceOverlay = lv_obj_create(NULL);

    lv_obj_clear_flag(ui_screenVoiceOverlay, LV_OBJ_FLAG_SCROLLABLE);

    /* Dark semi-transparent background */
    lv_obj_set_style_bg_color(ui_screenVoiceOverlay, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(ui_screenVoiceOverlay, 180, LV_PART_MAIN);

    /* Center panel */
    ui_voicePanel = lv_obj_create(ui_screenVoiceOverlay);

    lv_obj_set_size(ui_voicePanel, 260, 120);
    lv_obj_center(ui_voicePanel);

    lv_obj_set_style_radius(ui_voicePanel, 20, LV_PART_MAIN);
    lv_obj_set_style_bg_color(ui_voicePanel, lv_color_hex(0x111111), LV_PART_MAIN);

    lv_obj_set_style_shadow_color(ui_voicePanel, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_shadow_width(ui_voicePanel, 20, LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(ui_voicePanel, 120, LV_PART_MAIN);

    /* Label */
    ui_voiceLabel = lv_label_create(ui_voicePanel);

    lv_label_set_text(ui_voiceLabel, "Listening from your device...");
    lv_obj_center(ui_voiceLabel);

    lv_obj_set_style_text_color(ui_voiceLabel, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(ui_voiceLabel, &lv_font_montserrat_18, LV_PART_MAIN);
}

/* =========================
   DESTROY
   ========================= */

void ui_screenVoiceOverlay_screen_destroy(void)
{
    if(ui_screenVoiceOverlay)
    {
        lv_obj_del(ui_screenVoiceOverlay);
    }

    ui_screenVoiceOverlay = NULL;
    ui_voicePanel = NULL;
    ui_voiceLabel = NULL;
}
