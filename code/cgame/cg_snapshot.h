#ifndef cg_snapshot_h_included
#define cg_snapshot_h_included

void CG_ResetEntity( centity_t *cent );
void CG_ResetTimeChange (int serverTime, int ioverf);
void CG_ProcessSnapshots( void );
qboolean CG_GetSnapshot (int snapshotNumber, snapshot_t *snapshot);
qboolean CG_PeekSnapshot (int snapshotNumber, snapshot_t *snapshot);

#endif  // cg_snapshot_h_included
