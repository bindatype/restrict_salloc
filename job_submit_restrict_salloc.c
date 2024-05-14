#include <slurm/slurm.h>
#include <slurm/slurm_errno.h>

#include "src/slurmctld/slurmctld.h"

/*
 * These variables are required by the generic plugin interface.  If they
 * are not found in the plugin, the plugin loader will ignore it.
 *
 * plugin_name - a string giving a human-readable description of the
 * plugin.  There is no maximum length, but the symbol must refer to
 * a valid string.
 *
 * plugin_type - a string suggesting t#include <time.h>he type of the plugin or its
 * applicability to a particular form of data or method of data handling.
 * If the low-level plugin API is used, the contents of this string are
 * unimportant and may be anything.  Slurm uses the higher-level plugin
 * interface which requires this string to be of the form
 *
 *	<application>/<method>
 *
 * where <application> is a description of the intended application of
 * the plugin (e.g., "auth" for Slurm authentication) and <method> is a
 * description of how this plugin satisfies that application.  Slurm will
 * only load authentication plugins if the plugin_type string has a prefix
 * of "auth/".
 *
 * plugin_version - an unsigned 32-bit integer containing the Slurm version
 * (major.minor.micro combined into a single number).
 */

#define MAX_PARTITIONS 100
#define MAX_PARTITION_NAME_LENGTH 64
#define MAX_PARTITION_MAP_LENGTH 500

const char plugin_name[] = "Restrict salloc limit jobsubmit plugin";
const char plugin_type[] = "job_submit/restrict_salloc";
const uint32_t plugin_version = SLURM_VERSION_NUMBER;

int
contains_salloc (const char *str)
{
//	return strstr (str, "salloc") != NULL;
      return strstr(str, "salloc") != NULL || strstr(str, "ondemand/data/sys/dashboard") != NULL;
}

int
check_group_whitelist (const char *filename, job_desc_msg_t *job_desc)
{
	FILE *file = fopen (filename, "r");
	if (file == NULL)
	{
		info ("Failed to open salloc whitelist file %s", filename);
		return 2;
	}

	char *line = NULL;
	size_t len = 0;
	ssize_t read;
	int needed = snprintf (NULL, 0, "%u", job_desc->group_id);

	if (needed < 0)
	{
		info ("Error calculating the salloc group_id buffer size");
		return EXIT_FAILURE;
	}

	char *group_id = malloc (needed + 1);
	if (group_id == NULL)
	{
		info ("Failed to allocate salloc group_id memory");
		return EXIT_FAILURE;
	}

	snprintf (group_id, needed + 1, "%u", job_desc->group_id);
	while ((read = getline (&line, &len, file)) != -1)
	{
		if (strstr (line, group_id) != NULL)
		{
			info ("Found matching whitelist group_id: %s", line);
			free (line);
			fclose (file);
			return 0;
		} else {
			info ("Did not find matching whitelist group_id: %s", line);
		}
	}

	free (line);
	fclose (file);
	return 1;
}

bool is_partition_allowed(const char* partition) {
    char* allowed_partitions[MAX_PARTITIONS];
    char line[MAX_PARTITION_NAME_LENGTH];     
    int num_partitions = 0;                  
    FILE* file = fopen("/cm/shared/apps/slurm/var/etc/allowed_partitions.txt", "r"); 

    if (!file) {
        info("Failed to open allowed partitions file");
        return false; 
    }

    while (fgets(line, sizeof(line), file) && num_partitions < MAX_PARTITIONS) {
        line[strcspn(line, "\n")] = 0; 
        allowed_partitions[num_partitions] = strdup(line);
        if (!allowed_partitions[num_partitions]) { 
            info("Memory allocation failed for allowed partitions");
            while (num_partitions-- > 0) free(allowed_partitions[num_partitions]); 
            fclose(file); 
            return false;
        }
        num_partitions++;
    }
    fclose(file);  

    bool is_allowed = false;
    for (int i = 0; i < num_partitions; i++) {
        if (strcmp(partition, allowed_partitions[i]) == 0) {
            is_allowed = true;
            break;
        }
    }

    for (int i = 0; i < num_partitions; i++) {
        free(allowed_partitions[i]);
    }

    return is_allowed; 
}

bool is_user_allowed_for_partition(const char* partition, uint32_t group_id) {
    char line[MAX_PARTITION_MAP_LENGTH];
    char* mapped_group_id,*mapped_partition;
    FILE* file = fopen("/cm/shared/apps/slurm/var/etc/group_partition_map.txt", "r");
    if (!file) {
        info("Failed to open group_partition_map.txt");
        return false;
    }
    while (fgets(line, sizeof(line), file)) {
        mapped_group_id = strtok(line, ":");
        if (atoi(mapped_group_id) == group_id) {
            mapped_partition = strtok(NULL, ", \n");
            while (mapped_partition ){
                if (strcmp(mapped_partition, partition) == 0) {
                    fclose(file);
		    return true;
                }
                mapped_partition = strtok(NULL, ", \n");
            }
        }
    }
    fclose(file);
    return false;
}

extern int job_submit(job_desc_msg_t *job_desc, uint32_t submit_uid, char **err_msg) {
    if (!contains_salloc(job_desc->submit_line)) {
        info("sbatch/srun processed %s for userid %u on partition %s", job_desc->submit_line, submit_uid, job_desc->partition);
        return SLURM_SUCCESS;
    }

    if (!is_partition_allowed(job_desc->partition) && !is_user_allowed_for_partition(job_desc->partition, job_desc->group_id)) {
        info("Salloc not allowed on partition %s for uid:%u", job_desc->partition, submit_uid);
        return EXIT_FAILURE;
    }

    info("Salloc processed for userid %u on partition %s", submit_uid, job_desc->partition);
    return SLURM_SUCCESS;
}


int
job_modify (job_desc_msg_t *job_desc, job_record_t *job_ptr,
		uint32_t submit_uid, char **err_msg)
{
	// I HAVEN'T DONE ANYTHING WITH THIS YET. BUT, THEN AGAIN, ONLY SLURM ADMINS CAN MODIFY JOBS

	return SLURM_SUCCESS;
}
