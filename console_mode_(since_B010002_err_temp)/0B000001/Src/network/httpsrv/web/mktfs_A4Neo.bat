del web_pages_Neo\*.bak
del tfs_data_A4Neo.c
mk_tfs_py1 web_pages_Neo 

echo "D:\Freescale\Freescale_MQX_4_0\tools\mktfs.exe" web_pages
echo "D:\Freescale\Freescale_MQX_4_0\tools\mktfs.exe" web_pages_reduce
echo "D:\Freescale\Freescale_MQX_4_0\tools\mktfs.exe" web_pages_K64_minify
echo rename tfs_data.c tfs_data_A4P.c

python mkQ1HttpFs.py -f web_pages_Neo_minify -o tfs_data_A4Neo.c -t 6

pause
