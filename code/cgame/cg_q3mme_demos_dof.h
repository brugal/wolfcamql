#ifndef cg_q3mme_demos_dof_h_included
#define cg_q3mme_demos_dof_h_included

void CG_Q3mmeDemoDofCommand_f (void);
qboolean CG_Q3mmeDofParse( BG_XMLParse_t *parse, const struct BG_XMLParseBlock_s *fromBlock, void *data);
void CG_Q3mmeDofSave ( fileHandle_t fileHandle );
void CG_Q3mmeDofUpdate( int time, float timeFraction );

#endif  // cg_q3mme_demos_dof_h_included
