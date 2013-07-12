#include "cg_local.h"

#include "wolfcam_servercmds.h"

#include "wolfcam_local.h"

void Wolfcam_ScoreData (void)
{
    const score_t *sc;
    wclient_t *wc;
    //clientInfo_t *ci;
    int i;

    for (i = 0;  i < cg.numScores;  i++) {
        sc = &cg.scores[i];
        wc = &wclients[sc->client];
        if (!wc->currentValid)
            continue;
        //ci = &cgs.clientinfo[sc->client];
        //wc->ping = sc->ping;
        wc->serverPingAccum += sc->ping;
        wc->serverPingSamples++;
        //wc->time = sc->time;
        //wc->score = sc->score;
    }
}
