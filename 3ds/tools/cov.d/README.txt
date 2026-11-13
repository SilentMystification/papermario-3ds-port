Census lane game files (PM_FNCOV=1 only). One game source per line, repo-relative, starting with src/.
One file per row (<row>.txt). Read only by 3ds/CMakeLists.txt inside if(PM_FNCOV): each src/ line joins FNCOV_TUS
(-finstrument-functions). The normal build never reads these lists. Lines not starting with src/ are ignored.
