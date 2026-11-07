#include "ui.h"
#include "lvgl.h"
#include "ui_assets.h"
#include <stdio.h>
static lv_obj_t *header,*body,*wallpaper,*home,*selection,*contact_list,*rows[5],*names[5],*faces[5];
static lv_obj_t *call_face,*call_name,*call_state,*call_logo,*soft[3];
static lv_obj_t *header_bar,*footer_bar;
static const lv_image_dsc_t *icons[9]={&ui_icon_email,&ui_icon_texts,&ui_icon_checklist,&ui_icon_save,&ui_icon_contacts,&ui_icon_settings,&ui_icon_camera,&ui_icon_internet,&ui_icon_trackify};
static const lv_image_dsc_t *portraits[6]={&ui_portrait_franklin,&ui_portrait_trevor,&ui_portrait_lester,&ui_portrait_amanda,&ui_portrait_jimmy,&ui_portrait_dave};
static lv_obj_t *box(lv_obj_t *parent,int x,int y,int w,int h,uint32_t colour) {
    lv_obj_t *o=lv_obj_create(parent);lv_obj_remove_style_all(o);lv_obj_set_pos(o,x,y);lv_obj_set_size(o,w,h);
    lv_obj_set_style_bg_color(o,lv_color_hex(colour),0);lv_obj_set_style_bg_opa(o,LV_OPA_COVER,0);lv_obj_remove_flag(o,LV_OBJ_FLAG_SCROLLABLE);return o;
}
static lv_obj_t *text(lv_obj_t *parent,int x,int y,int w,const char *value) {
    lv_obj_t *o=lv_label_create(parent);lv_obj_set_pos(o,x,y);lv_obj_set_width(o,w);lv_label_set_text(o,value);return o;
}
static lv_obj_t *image(lv_obj_t *parent,int x,int y,const lv_image_dsc_t *source) {
    lv_obj_t *o=lv_image_create(parent);lv_image_set_src(o,source);lv_obj_set_pos(o,x,y);return o;
}
static void visible(lv_obj_t *o,bool show) { if(show)lv_obj_remove_flag(o,LV_OBJ_FLAG_HIDDEN);else lv_obj_add_flag(o,LV_OBJ_FLAG_HIDDEN); }
void phone_ui_init(void) {
    lv_obj_t *s=lv_screen_active();lv_obj_set_style_bg_color(s,lv_color_black(),0);lv_obj_set_style_text_color(s,lv_color_white(),0);
    lv_obj_set_style_text_font(s,&lv_font_montserrat_14,0);lv_obj_remove_flag(s,LV_OBJ_FLAG_SCROLLABLE);
    text(s,5,6,70,LV_SYMBOL_WIFI);lv_obj_t *clock=text(s,78,6,85,"09:41");lv_obj_set_style_text_align(clock,LV_TEXT_ALIGN_CENTER,0);
    text(s,201,6,36,LV_SYMBOL_BATTERY_FULL);
    header_bar=box(s,0,30,240,30,0x5a97e8);header=text(header_bar,3,4,234,"");lv_obj_set_style_text_align(header,LV_TEXT_ALIGN_CENTER,0);
    lv_obj_set_style_text_font(header,&lv_font_montserrat_20,0);
    wallpaper=image(s,0,60,ui_wallpapers[0]);
    home=box(s,0,60,240,223,0);lv_obj_set_style_bg_opa(home,LV_OPA_TRANSP,0);
    selection=box(home,0,0,68,65,0x98b6e6);lv_obj_set_style_bg_opa(selection,100,0);
    for(int i=0;i<9;i++) image(home,16+(i%3)*80,10+(i/3)*70,icons[i]);
    contact_list=box(s,0,60,240,223,0xd2d2d2);
    for(int i=0;i<5;i++) {
        rows[i]=box(contact_list,0,i*44,240,44,0xebebeb);
        faces[i]=image(rows[i],3,2,portraits[0]);lv_image_set_pivot(faces[i],0,0);lv_image_set_scale(faces[i],160);
        names[i]=text(rows[i],46,13,190,"");lv_obj_set_style_text_color(names[i],lv_color_black(),0);
    }
    body=text(s,10,75,220,"");
    call_face=image(s,9,72,portraits[0]);call_name=text(s,82,73,152,"");call_state=text(s,82,104,152,"");
    call_logo=text(s,74,183,100,LV_SYMBOL_CALL);lv_obj_set_style_text_font(call_logo,&lv_font_montserrat_20,0);
    footer_bar=box(s,0,283,240,37,0x202a3b);
    soft[0]=image(footer_bar,27,7,&ui_soft_select);soft[1]=image(footer_bar,108,7,&ui_soft_keypad);soft[2]=image(footer_bar,188,7,&ui_soft_back);
}
void phone_ui_render(const phone_t *p) {
    bool ishome=p->page==PHONE_HOME,iscontacts=p->page==PHONE_CONTACTS;
    bool issettings=p->page==PHONE_SETTINGS || p->page==PHONE_SETTING_OPTIONS;
    bool islist=p->page==PHONE_ROLE || p->page==PHONE_CHECKLIST || p->page==PHONE_OPEN_MISSIONS || p->page==PHONE_PROGRESS;
    bool iscall=p->page==PHONE_CALL || p->page==PHONE_INCOMING || p->page==PHONE_DIALING || p->page==PHONE_BUSY;
    const uint32_t *palette=theme_colors[p->settings[SET_THEME]];
    lv_obj_set_style_bg_color(header_bar,lv_color_hex(palette[1]),0);
    lv_obj_set_style_bg_color(footer_bar,lv_color_hex(palette[2]),0);
    lv_obj_set_style_bg_color(selection,lv_color_hex(palette[0]),0);
    lv_image_set_src(wallpaper,ui_wallpapers[p->settings[SET_BACKGROUND]]);
    visible(wallpaper,ishome);visible(home,ishome);visible(contact_list,iscontacts||issettings||islist);visible(body,!ishome&&!iscontacts&&!iscall&&!issettings&&!islist);
    visible(call_face,iscall);visible(call_name,iscall);visible(call_state,iscall);visible(call_logo,iscall);
    lv_image_set_src(soft[0],iscall?&ui_soft_hangup:&ui_soft_select);visible(soft[1],iscontacts);lv_image_set_src(soft[2],&ui_soft_back);
    if(ishome) {
        lv_label_set_text(header,phone_apps[p->selected_app]);
        lv_obj_set_pos(selection,6+(p->selected_app%3)*80,2+(p->selected_app/3)*70);
        for(unsigned i=0;i<9;i++) {
            lv_obj_t *icon=lv_obj_get_child(home,(int)i+1);
            visible(icon,i!=8 || p->trackify_unlocked);
            lv_image_set_scale(icon,i==p->selected_app?307:256); /* Original HOMEMENU_IFRUIT: active icon 120%. */
        }
    } else if(iscontacts) {
        lv_label_set_text(header,"Contacts");unsigned first=p->selected_contact<5?0:p->selected_contact-4;
        for(unsigned i=0;i<5;i++) {
            unsigned n=first+i;visible(rows[i],n<6);if(n>=6)continue;
            bool chosen=n==p->selected_contact;
            lv_obj_set_style_bg_color(rows[i],lv_color_hex(chosen?palette[1]:0xebebeb),0);
            visible(faces[i],true);lv_obj_set_x(names[i],46);lv_obj_set_width(names[i],190);
            lv_obj_set_style_text_color(names[i],chosen?lv_color_white():lv_color_black(),0);
            lv_label_set_text(names[i],phone_contacts[n]);lv_image_set_src(faces[i],portraits[n]);
        }
    } else if(issettings) {
        bool options=p->page==PHONE_SETTING_OPTIONS;
        unsigned selected=options?p->selected_option:p->selected_setting;
        unsigned count=options?setting_counts[p->selected_setting]:SET_COUNT;
        unsigned first=selected<5?0:selected-4;
        lv_label_set_text(header,options?setting_options[p->selected_setting][selected]:"Settings");
        for(unsigned i=0;i<5;i++) {
            unsigned n=first+i;visible(rows[i],n<count);if(n>=count)continue;
            visible(faces[i],false);lv_obj_set_x(names[i],6);lv_obj_set_width(names[i],228);
            bool chosen=n==selected;
            lv_obj_set_style_bg_color(rows[i],lv_color_hex(chosen?palette[1]:0xebebeb),0);
            lv_obj_set_style_text_color(names[i],chosen?lv_color_white():lv_color_black(),0);
            const char *name=options?setting_options[p->selected_setting][n]:setting_names[n];
            if(options && n==p->settings[p->selected_setting])lv_label_set_text_fmt(names[i],"%s %s",LV_SYMBOL_OK,name);
            else lv_label_set_text(names[i],name);
        }
    } else if(islist) {
        unsigned count=0,selected=0;
        const char *title="";
        if(p->page==PHONE_ROLE){count=3;selected=p->selected_role;title="Character";}
        else if(p->page==PHONE_CHECKLIST){count=2;selected=p->selected_list;title="Check List";}
        else if(p->page==PHONE_OPEN_MISSIONS){count=world_count(&p->world,false);selected=p->selected_task;title=checklist_labels[0];}
        else {count=5;title=checklist_labels[1];}
        lv_label_set_text(header,title);
        for(unsigned i=0;i<5;i++) {
            bool empty=p->page==PHONE_OPEN_MISSIONS && count==0;
            visible(rows[i],i<count || (empty&&i==0));if(i>=count&&!empty)continue;
            visible(faces[i],false);lv_obj_set_x(names[i],6);lv_obj_set_width(names[i],228);
            bool chosen=i==selected && !empty;
            lv_obj_set_style_bg_color(rows[i],lv_color_hex(chosen?palette[1]:0xebebeb),0);
            lv_obj_set_style_text_color(names[i],chosen?lv_color_white():lv_color_black(),0);
            if(p->page==PHONE_ROLE)lv_label_set_text(names[i],world_roles[i]);
            else if(p->page==PHONE_CHECKLIST)lv_label_set_text(names[i],checklist_labels[i]);
            else if(p->page==PHONE_OPEN_MISSIONS){int task=world_task(&p->world,i);lv_label_set_text(names[i],task<0?"No open missions":mission_names[task]);}
            else lv_label_set_text_fmt(names[i],"%s: %u",checklist_labels[i+2],i==0?world_count(&p->world,true):0);
        }
    } else if(iscall) {
        lv_label_set_text(header,phone_contacts[p->selected_contact]);lv_label_set_text(call_name,phone_contacts[p->selected_contact]);
        lv_image_set_src(call_face,portraits[p->selected_contact]);
        if(p->page==PHONE_INCOMING)lv_label_set_text(call_state,"INCOMING CALL");
        else if(p->page==PHONE_DIALING)lv_label_set_text(call_state,"DIALING...");
        else if(p->page==PHONE_BUSY)lv_label_set_text(call_state,"BUSY");
        else lv_label_set_text_fmt(call_state,"%02lu:%02lu",(unsigned long)p->call_seconds/60,(unsigned long)p->call_seconds%60);
    } else {
        lv_label_set_text(header,p->page==PHONE_TASK_DETAIL?"Mission":p->page==PHONE_MESSAGES?"Texts":phone_apps[p->selected_app]);
        if(p->page==PHONE_TASK_DETAIL){int task=world_task(&p->world,p->selected_task);lv_label_set_text_fmt(body,"%s\n\nLocal replay\n\nOK: complete simulation\nHold OK: back",task>=0?mission_names[task]:"");}
        else if(p->page==PHONE_MESSAGES && world_count(&p->world,false)){int task=world_task(&p->world,0);lv_label_set_text_fmt(body,"%s\n\n%s\n\nLocal replay available\nin Check List",world_roles[p->world.role],mission_names[task]);}
        else lv_label_set_text(body,p->page==PHONE_MESSAGES?"No messages":"");
    }
}
