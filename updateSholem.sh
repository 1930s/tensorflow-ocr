# to update pages that have not been edited yet.

# make sholem-b

rm -f /tmp/toupdate
# name the latest-changed unedited file for -newer.
find SholemAleykhem \! -newer SholemAleykhem/202592/0110 \
    -name '[0-9][0-9][0-9][0-9]' | \
	sed -e 's/SholemAleykhem/cp &{Update,}/' > /tmp/toupdate

