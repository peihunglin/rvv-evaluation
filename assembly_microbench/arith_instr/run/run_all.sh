#!/bin/bash
script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)

script -q -e -c "${script_dir}/run_scalar_int.sh" "k3_report/k3_scalar_int.txt"
script -q -e -c "${script_dir}/run_scalar_fp.sh"  "k3_report/k3_scalar_fp.txt"
script -q -e -c "${script_dir}/run_vector_int.sh" "k3_report/k3_vector_int.txt"
script -q -e -c "${script_dir}/run_vector_fp.sh"  "k3_report/k3_vector_fp.txt"
