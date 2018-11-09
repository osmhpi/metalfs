pipeline {
  agent {
    node {
      label 'vivado'
    }

  }
  stages {
    stage('PSLSE') {
      steps {
        dir(path: 'pslse') {
          git(url: 'https://github.com/ibm-capi/pslse', branch: 'master')
        }

      }
    }
    stage('SNAP') {
      steps {
        dir(path: 'snap') {
          git(url: 'https://github.com/rs22/snap', branch: 'bd_helloworld')
          writeFile(text: '''#
# Automatically generated file; DO NOT EDIT.
# Kernel Configuration
#
N250S=y
# ADKU3 is not set
# AD8K5 is not set
# S121B_BPIx16 is not set
# S121B_SPIx4 is not set
# N250SP is not set
# RCXVUP is not set
FPGACARD="N250S"
FLASH_INTERFACE="BPIx16"
FLASH_SIZE="64"
FLASH_USERADDR="0x01000000"
# S121B is not set
CAPI10=y
# CAPI20 is not set
FPGACHIP="xcku060-ffva1156-2-e"
NUM_OF_ACTIONS=1
# HLS_ACTION is not set
HDL_ACTION=y
# HDL_EXAMPLE is not set
# HDL_NVME_EXAMPLE is not set
# HLS_MEMCOPY is not set
# HLS_SPONGE is not set
# HLS_HASHJOIN is not set
# HLS_SEARCH is not set
# HLS_BFS is not set
# HLS_INTERSECT is not set
# HLS_NVME_MEMCOPY is not set
# HLS_LATENCY_EVAL is not set
# HLS_HELLOWORLD is not set
# ENABLE_HLS_SUPPORT is not set
HLS_SUPPORT="FALSE"
DISABLE_SDRAM_AND_BRAM=y
# FORCE_SDRAM_OR_BRAM is not set
SDRAM_USED="FALSE"
BRAM_USED="FALSE"
DDR3_USED="FALSE"
DDR4_USED="FALSE"
DDRI_USED="FALSE"
DISABLE_NVME=y
# FORCE_NVME is not set
NVME_USED="FALSE"
SIM_XSIM=y
# SIM_IRUN is not set
# SIM_XCELIUM is not set
# SIM_MODELSIM is not set
# SIM_QUESTA is not set
# NO_SIM is not set
SIMULATOR="xsim"
DENALI_USED="FALSE"

#
# ================= Advanced Options: =================
#
# ENABLE_PRFLOW is not set
USE_PRFLOW="FALSE"
# ENABLE_ILA is not set
ILA_DEBUG="FALSE"
# ENABLE_FACTORY is not set
FACTORY_IMAGE="FALSE"
CLOUD_USER_FLOW="FALSE"
CLOUD_BUILD_BITFILE="FALSE"
''', file: 'defconfig/N250S.hls_metalfpga.defconfig')
          sh '''#! /bin/bash
source /opt/Xilinx/Vivado/2018.2/settings64.sh
rm -f snap_env.sh
echo "export TIMING_LABLIMIT=\\"-200\\"" >> snap_env.sh
echo "export PSLVER=8" >> snap_env.sh
echo "export PSLSE_ROOT=$WORKSPACE/pslse" >> snap_env.sh
echo "export ACTION_ROOT=$WORKSPACE/src/metal_fpga" >> snap_env.sh
make clean_config N250S.hls_metalfpga.defconfig software'''
        }

      }
    }
    stage('Build') {
      parallel {
        stage('Hardware') {
          steps {
            dir(path: 'snap') {
              sh '''#! /bin/bash
source /opt/Xilinx/Vivado/2018.2/settings64.sh
make hw_project
'''
            }

          }
        }
        stage('Software') {
          environment {
            PSLSE_ROOT = '${WORKSPACE}/pslse'
            SNAP_ROOT = '${WORKSPACE}/snap'
          }
          steps {
            cmakeBuild(installation: 'InSearchPath', buildDir: 'build', buildType: 'Debug')
            dir(path: 'build') {
              sh '''#! /bin/bash
make -j8'''
            }

          }
        }
        stage('Software - InMemory') {
          environment {
            PSLSE_ROOT = '${WORKSPACE}/pslse'
            SNAP_ROOT = '${WORKSPACE}/snap'
          }
          steps {
            cmakeBuild(installation: 'InSearchPath', buildDir: 'build-inmemory', cmakeArgs: '-D METAL_USE_INMEMORY_STORAGE=ON', buildType: 'Debug')
            dir(path: 'build-inmemory') {
              sh '''#! /bin/bash
make -j8'''
            }

          }
        }
      }
    }
    stage('Test') {
      parallel {
        stage('metal_test') {
          steps {
            dir(path: 'build-inmemory') {
              sh '''#! /bin/bash
./metal_test'''
            }

          }
        }
        stage('Sim') {
          environment {
            PSLSE_ROOT = '${WORKSPACE}/pslse'
          }
          steps {
            dir(path: 'snap') {
              sh '''#! /bin/bash
source /opt/Xilinx/Vivado/2018.2/settings64.sh
make model
'''
            }
            dir(path: 'snap/hardware/sim/xsim') {
              sh '''#! /bin/bash
source /opt/Xilinx/Vivado/2018.2/settings64.sh
echo "snap_maint" > ../testlist.sh
echo "$WORKSPACE/build/metal_pipeline_test --gtest_filter=*ReadWrite*" >> ../testlist.sh
chmod +x ../testlist.sh
../run_sim -explore -list testlist.sh -noaet'''
            }

          }
        }
      }
    }
  }
  environment {
    XILINXD_LICENSE_FILE = '/var/jenkins-agent/Xilinx.lic'
  }
}
