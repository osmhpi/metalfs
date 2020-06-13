export VIVADO_EDITION ?= webpack
export PSL ?= PSL8
PSL_TAG = $(shell echo $(PSL) | tr A-Z a-z)
METAL_TAG ?= $(shell if [ -z "`git status --porcelain`" ]; then git rev-parse --short HEAD; else echo dirty; fi)
export METAL_TAG := ${METAL_TAG}

docker_build := docker build --build-arg VIVADO_EDITION --build-arg PSL --build-arg METAL_TAG

.PHONY: runtime sdk-base sdk

all-sdk: sdk-base sdk
all-runtime: runtime

git-is-clean:
ifeq '${shell git status --porcelain}' ''
	@ # git is clean
else
	${error Git status is not clean.}
endif


latest-sdk: git-is-clean all-sdk
	docker tag metalfs/sdk-base:${METAL_TAG}-${VIVADO_EDITION} 	 metalfs/sdk-base:${VIVADO_EDITION}
	docker tag metalfs/sdk:${METAL_TAG}-${VIVADO_EDITION}-${PSL_TAG} metalfs/sdk:${VIVADO_EDITION}-${PSL_TAG}
lastest-runtime: git-is-clean all-runtime
	docker tag metalfs/runtime:${METAL_TAG}                      metalfs/runtime:latest

sdk-base:
	${docker_build} \
		-t metalfs/sdk-base:${METAL_TAG}-${VIVADO_EDITION}-${PSL_TAG} \
		-f docker/sdk-base/Dockerfile \
		..

sdk:
	${docker_build} \
		-t metalfs/sdk:${METAL_TAG}-${VIVADO_EDITION} \
		-f docker/sdk/Dockerfile \
		..

runtime:
	${docker_build} \
		-t metalfs/runtime:${METAL_TAG} \
		-f docker/runtime/Dockerfile \
		..

pull-latest-sdk:
	docker pull metalfs/sdk-base:${VIVADO_EDITION}
	docker pull metalfs/sdk:${VIVADO_EDITION}-${PSL_TAG}
pull-latest-runtime:
	docker pull metalfs/runtime:latest

push-sdk:
	docker push metalfs/sdk-base:${METAL_TAG}
	docker push metalfs/sdk:${METAL_TAG}
push-runtime:
	docker push metalfs/runtime:${METAL_TAG}

push-latest-sdk: git-is-clean push-sdk
	docker push metalfs/sdk-base:${VIVADO_EDITION}
	docker push metalfs/sdk:${VIVADO_EDITION}-${PSL_TAG}
push-latest-runtime: git-is-clean push-runtime
	docker push metalfs/runtime:latest
