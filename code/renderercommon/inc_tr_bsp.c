

void R_LoadAdvertisements( lump_t *l )
{
	dadvertisement_t *ads;
	int i, count;

	ads = (void *)(fileBase + l->fileofs);
	if (l->filelen % sizeof(*ads)) {
		ri.Error (ERR_DROP, "R_LoadAdvertisements()  LoadMap: funny lump size in %s",s_worldData.name);
	}
	count = l->filelen / sizeof(*ads);

	s_worldData.numAds = 0;
	Com_Memset(s_worldData.adShaders, 0, sizeof(s_worldData.adShaders));

	//ri.Printf(PRINT_ALL, "%d ads in map\n", count);

	if (count > MAX_MAP_ADVERTISEMENTS) {
		ri.Printf(PRINT_ALL, "count > MAX_MAP_ADVERTISEMENTS\n");
		return;
	}

	if (!count) {
		return;
	}

	s_worldData.numAds = count;

	for (i = 0;  i < count;  i++, ads++) {
#if 1
		ri.Printf(PRINT_ALL, "Advertisement %d:\n", i + 1);
		ri.Printf(PRINT_ALL, "  cellId: %d\n", LittleLong(ads->cellId));
		ri.Printf(PRINT_ALL, "  normal: %f %f %f\n", LittleFloat(ads->normal[0]), LittleFloat(ads->normal[1]), LittleFloat(ads->normal[2]));
		ri.Printf(PRINT_ALL, "  rect[0]:  %f %f %f\n", LittleFloat(ads->rect[0][0]), LittleFloat(ads->rect[0][1]), LittleFloat(ads->rect[0][2]));
		ri.Printf(PRINT_ALL, "  rect[1]:  %f %f %f\n", LittleFloat(ads->rect[1][0]), LittleFloat(ads->rect[1][1]), LittleFloat(ads->rect[1][2]));
		ri.Printf(PRINT_ALL, "  rect[2]:  %f %f %f\n", LittleFloat(ads->rect[2][0]), LittleFloat(ads->rect[2][1]), LittleFloat(ads->rect[2][2]));
		ri.Printf(PRINT_ALL, "  rect[3]:  %f %f %f\n", LittleFloat(ads->rect[3][0]), LittleFloat(ads->rect[3][1]), LittleFloat(ads->rect[3][2]));
		ri.Printf(PRINT_ALL, "  model:  %s\n", ads->model);
#endif

		s_worldData.ads[i].cellId = LittleLong(ads->cellId);

		s_worldData.ads[i].normal[0] = LittleFloat(ads->normal[0]);
		s_worldData.ads[i].normal[1] = LittleFloat(ads->normal[1]);
		s_worldData.ads[i].normal[2] = LittleFloat(ads->normal[2]);

		s_worldData.ads[i].rect[0][0] = LittleFloat(ads->rect[0][0]);
		s_worldData.ads[i].rect[0][1] = LittleFloat(ads->rect[0][1]);
		s_worldData.ads[i].rect[0][2] = LittleFloat(ads->rect[0][2]);

		s_worldData.ads[i].rect[1][0] = LittleFloat(ads->rect[1][0]);
		s_worldData.ads[i].rect[1][1] = LittleFloat(ads->rect[1][1]);
		s_worldData.ads[i].rect[1][2] = LittleFloat(ads->rect[1][2]);

		s_worldData.ads[i].rect[2][0] = LittleFloat(ads->rect[2][0]);
		s_worldData.ads[i].rect[2][1] = LittleFloat(ads->rect[2][1]);
		s_worldData.ads[i].rect[2][2] = LittleFloat(ads->rect[2][2]);

		s_worldData.ads[i].rect[3][0] = LittleFloat(ads->rect[3][0]);
		s_worldData.ads[i].rect[3][1] = LittleFloat(ads->rect[3][1]);
		s_worldData.ads[i].rect[3][2] = LittleFloat(ads->rect[3][2]);

		Q_strncpyz(s_worldData.ads[i].model, ads->model, sizeof(s_worldData.ads[i].model));

#if 1
		{
			int width, height;
			float scale;

			//width = Distance(verts[3].xyz, verts[0].xyz);
			width = Distance(s_worldData.ads[i].rect[3], s_worldData.ads[i].rect[0]);

			//height = Distance(verts[3].xyz, verts[2].xyz);
			height = Distance(s_worldData.ads[i].rect[3], s_worldData.ads[i].rect[2]);

			if (height > 0.0) {
				scale = (float)width / (float)height;
			} else {
				scale = 0;
			}
			ri.Printf(PRINT_ALL, "  %d x %d   (%f)\n", width, height, scale);
		}
#endif
	}
}
