#include "qr_mapper.h"
#include "ui.h"
#include <ctype.h>
#include <string.h>

const lv_img_dsc_t * get_qr_image_for_location(
    const char *rack,
    int row,
    int column,
    const char *side
)
{
    char r = tolower(rack[0]);
    char s = (tolower(side[0]) == 'l') ? 'l' : 'r';

    if(r == 'a')
    {
        if(row==1 && column==1) return s=='l'? &ui_img_qr_ar1c1l_png : &ui_img_qr_ar1c1r_png;
        if(row==1 && column==2) return s=='l'? &ui_img_qr_ar1c2l_png : &ui_img_qr_ar1c2r_png;
        if(row==2 && column==1) return s=='l'? &ui_img_qr_ar2c1l_png : &ui_img_qr_ar2c1r_png;
        if(row==2 && column==2) return s=='l'? &ui_img_qr_ar2c2l_png : &ui_img_qr_ar2c2r_png;
    }

    if(r == 'b')
    {
        if(row==1 && column==1) return s=='l'? &ui_img_qr_br1c1l_png : &ui_img_qr_br1c1r_png;
        if(row==1 && column==2) return s=='l'? &ui_img_qr_br1c2l_png : &ui_img_qr_br1c2r_png;
        if(row==2 && column==1) return s=='l'? &ui_img_qr_br2c1l_png : &ui_img_qr_br2c1r_png;
        if(row==2 && column==2) return s=='l'? &ui_img_qr_br2c2l_png : &ui_img_qr_br2c2r_png;
    }

    return NULL;
}
