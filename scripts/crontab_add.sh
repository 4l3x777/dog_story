#!/bin/bash

line="15 0 * * * ~/app/scripts/rotate_log.sh"
(crontab -u $(whoami) -l; echo "$line" ) | crontab -u $(whoami) -