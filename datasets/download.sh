for d in covid osm fb
do
# -c resumes an interrupted download; a complete file is left as it is
wget --no-check-certificate -c https://www.cse.cuhk.edu.hk/mlsys/gre/$d
done