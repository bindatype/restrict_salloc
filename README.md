REQUIRE:
/cm/shared/apps/slurm/var/etc/allowed_partitions.txt
	Single column list of partitions that anyone can salloc from. 

/cm/shared/apps/slurm/var/etc/group_partition_map.txt
	0: 384gb, altus_amnis
	2507: debug, debug-cpu, debug-gpu
	1007: secret

BUILD:
gcc -fPIC -DHAVE_CONFIG_H -I /c1/source/slurm-23.02.4 -g -O2 -pthread -fno-gcse -Werror -Wall -g -O0 -fno-strict-aliasing -MT job_submit_restrict_salloc.lo -MD -MP -MF .deps/job_submit_restrict_salloc.Tpo -c job_submit_restrict_salloc.c -o .libs/job_submit_restrict_salloc.o &&  gcc -shared -fPIC -DPIC .libs/job_submit_restrict_salloc.o -O2 -pthread -O0 -pthread -Wl,-soname -Wl,job_submit_restrict_salloc.so    -o job_submit_restrict_salloc.so && cp job_submit_restrict_salloc.so /cm/shared/apps/slurm/23.02.4a/lib/slurm/job_submit_restrict_salloc.so && systemctl restart slurmctld



sinfo | awk '{print $1}'| sort -u | awk '{print "strcmp(job_desc->partition, \""$1"\") != 0 &&"}'
 tail -f /var/log/slurm/slurmctld.log

find /c1/source/slurm-23.02.4/src/ -type f -exec grep -l alloc_sid {} \;
less /c1/source/slurm-23.02.4/src/slurmctld/slurmctld.h
