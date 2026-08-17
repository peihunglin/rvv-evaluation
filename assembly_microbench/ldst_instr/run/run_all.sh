#!/bin/bash
script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)

script -q -e -c "${script_dir}/run_scalar.sh"             "k3_report/k3_scalar.txt"
script -q -e -c "${script_dir}/run_vector_contiguous.sh"  "k3_report/k3_vector_contiguous.txt"
script -q -e -c "${script_dir}/run_vector_mask.sh"        "k3_report/k3_vector_mask.txt"
script -q -e -c "${script_dir}/run_vector_setvl.sh"       "k3_report/k3_vector_setvl.txt"
script -q -e -c "${script_dir}/run_vector_strided.sh"     "k3_report/k3_vector_strided.txt"
