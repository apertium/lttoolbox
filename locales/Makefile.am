lttoolbox.dat: root.txt en.txt es.txt
	genrb -d . root.txt en.txt es.txt
	echo root.res en.res es.res > package_list.txt
	pkgdata -p lttoolbox --mode archive -d . package_list.txt

localesdir = $(LOCALES_DIR)
dist_locales_DATA = lttoolbox.dat
