#define ___PINV___
#include "global.h"
#include "menuS.h"
#include "pinv.h"
#include "filer.h"
#include "showdialog.h"

static struct coordinate *p_i_list = NULL;
static struct p_i_l_type *p_invars = NULL;
static int      num_pinv = 0;

void ClearPinv(void) {
    struct p_i_l_type *from = p_invars;
    struct p_i_object *pip, *pin;

    if (p_invars == NULL)
        return;
    p_invars = p_invars->next;
    from->next = NULL;
    for (; p_invars != NULL; p_invars = from) {
        from = p_invars->next;
        pip = p_invars->i_l;
        free((char *) p_invars);
        for (; pip != NULL; pip = pin) {
            pin = pip->next;
            free((char *) pip);
        }
    }
}

void CollectPinvar(int complain) {
    FILE           *p_f;
    char            buf[LINEMAX], buf2[LINEMAX];
    struct p_i_object *pip;
    struct p_i_l_type *pil = NULL;
    int             i, j, m, np;
    struct place_object *plp;
    char string[300];



    strcpy(buf, GetCurrentFilename());
    strcpy(buf2, GetCurrentFilename());
    strcat(buf, ".pin");
    strcat(buf2, ".net");
    {
        struct stat     stb, stb2;

        if ((stat(buf, &stb) < 0) || (stat(buf2, &stb2) < 0)
                || (stb2.st_mtime > stb.st_mtime)) {
            if (complain) {
                sprintf(string, "No up-to-date P-invariants for net %s", GetCurrentFilename());
                ShowInfoDialog(string, frame_w);
            }
            return;
        }
    }
    if ((p_f = fopen(buf, "r")) == NULL) {
        if (complain) {
            sprintf(string, "Can't open file %s for read", buf);
            ShowErrorDialog(string, frame_w);
        }
        return;
    }
    ClearPinv();
    fscanf(p_f, "%d\n", &num_pinv);
    p_invars = NULL;
    if (num_pinv <= 0)
        return;
    for (i = num_pinv; i-- > 0;) {
        if (p_invars == NULL)
            p_invars = pil = (struct p_i_l_type *) emalloc(sizeof(struct p_i_l_type));
        else {
            pil->next = (struct p_i_l_type *) emalloc(sizeof(struct p_i_l_type));
            pil = pil->next;
        }
        fscanf(p_f, "%d", &j);
        pil->i_l = pip = NULL;
        for (; j-- > 0;) {
            if (pip == NULL)
                pil->i_l = pip = (struct p_i_object *) emalloc(sizeof(struct p_i_object));
            else {
                pip->next = (struct p_i_object *) emalloc(sizeof(struct p_i_object));
                pip = pip->next;
            }
            fscanf(p_f, "%d %d", &m, &np);
            for (plp = netobj->places; --np > 0; plp = plp->next);
            pip->mult = m;
            pip->p_p = plp;
        }
        pip->next = NULL;
    }
    pil->next = p_invars;
    (void) fclose(p_f);
}

static void HighlightPinv(struct place_object *place) {
    struct coordinate *c_p;

    c_p = (struct coordinate *) emalloc(COORD_SIZE);
    c_p->next = p_i_list;
    p_i_list = c_p;
    c_p->x = place->center.x;
    c_p->y = place->center.y;
}


void DehighlightPinv(void) {
    struct coordinate *n_p;

    if (p_i_list == NULL)
        return;
    dehighlight_list(p_i_list, (int)(place_radius + place_radius));
    for (; p_i_list != NULL; p_i_list = n_p) {
        n_p = p_i_list->next;
        free((char *) p_i_list);
    }
}


void ShowPinv(XButtonEvent *ie) {
    struct p_i_l_type *from;
    struct p_i_object *pip;
    int             first = TRUE;
    char            string[LINEMAX];

    StatusPrintf("");
    ShowShowDialog("");
    if (figure_modified)
        ClearPinv();
    if (p_invars == NULL) {
        ShowInfoDialog("Sorry, no up-to-date place invariants available", frame_w);
        return;
    }
    DehighlightPinv();
    fix_x = event_x(ie) / zoom_level;
    fix_y = event_y(ie) / zoom_level;
    if ((cur_place = near_place_obj(fix_x, fix_y)) == NULL) {
        return;
    }

    for (from = p_invars = p_invars->next; first || (p_invars != from);
            first = FALSE, p_invars = p_invars->next) {
        for (pip = p_invars->i_l; pip != NULL; pip = pip->next)
            if (pip->p_p == cur_place)
                goto found;
    }

    sprintf(string, "place %s is not contained in any P-invariant", cur_place->tag);
    ShowShowDialog(string);
    StatusPrintf(string);
    return;
found:
    DisplayPinv();
}

static void DisplayPinv(void) {
    struct p_i_object *pip;
    char            string[2000], buf[2000];
    int             sumtok = 0;
    int 			prevdim, dim;

    /*	StatusPrintf("SEE place invariants with LEFT button"); */
    string[0] = '\0';
    prevdim = 0;
    for (pip = p_invars->i_l; pip != NULL; pip = pip->next) {
        HighlightPinv(pip->p_p);
        if (pip != p_invars->i_l)
            strcat(string, " + ");

        if (pip->mult != 1) {
            sprintf(buf, "%d ", pip->mult);

            strcat(string, buf);
        }
        strcat(string, pip->p_p->tag);
        dim = (strlen(string)) / 60;

        if (dim > prevdim) {
            strcat(string, "\n");
            prevdim = dim;
        }
        if (pip->p_p->mpar == NULL)
            sumtok += (pip->p_p->m0 * pip->mult);
        else
            sumtok += (pip->p_p->mpar->value * pip->mult);

    }
    highlight_list(p_i_list, (int)(place_radius + place_radius), FALSE, NULL);
    sprintf(buf, " = %d", sumtok);
    strcat(string, buf);
    StatusPrintf(string);
    ShowShowDialog(string);
}
