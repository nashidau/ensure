#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#include <Eina.h>
#include <Elementary.h>

#define ensure_unused	__attribute__((unused))

struct error {
	const char *msg;
};


Evas_Object *window_add(void);

static void select_cb(void *data, Evas_Object *obj, void *event);

int
elm_main(int argc, char **argv){
        Evas_Object *win;

        if (argc < 2){
               printf("Usage: %s <program>\n", argv[0]);
                exit(0);
        }
        win = window_add();
        elm_list_go(win);
        elm_run();
        elm_shutdown();
        return 0;
}
ELM_MAIN()



Evas_Object *
window_add(void){
        Evas_Object *win,*bg,*li,*bx,*run,*cfg;

        win = elm_win_add(NULL, "Ensure", ELM_WIN_BASIC);
        elm_win_title_set(win, "Ensure");
        //evas_object_smart_callback_add(win,"delete,request", win_del,NULL);

        bg = elm_bg_add(win);
        evas_object_size_hint_weight_set(bg,1,1);
        elm_win_resize_object_add(win, bg);
        evas_object_show(bg);

        bx = elm_box_add(win);
        evas_object_size_hint_weight_set(bx, 1.0, 1.0);
       elm_win_resize_object_add(win, bg);
        evas_object_show(bg);

        bx = elm_box_add(win);
        evas_object_size_hint_weight_set(bx, 1.0, 1.0);
        elm_win_resize_object_add(win, bx);
        evas_object_show(bx);


  /* add a label widget, set the text and put it in the pad frame */

   lb = elm_label_add(win);
   elm_label_label_set(lb, "Books");
   elm_box_pack_end(bx,lb);
   evas_object_show(lb);

        li = elm_list_add(win);
        evas_object_size_hint_weight_set(li, 1, 1);
        evas_object_size_hint_align_set(li, -1, -1);
        elm_box_pack_end(bx, li);
        evas_object_show(li);

evas_object_resize(win, 300, 300);
evas_object_show(win);
        /* Urh.. the list widget for now */
        return li;
}
static void
select_cb(void *data ensure_unused, Evas_Object *obj ensure_unused, void *event){
        /* FIXME: We should use a peruse pointer here */
        //Evas_Object *li = data;
        Elm_List_Item *itm = event;
        struct error *err;

        err = elm_list_item_data_get(itm);
}


