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
	docker tag metalfs/sdk-base:${METAL_TAG}-${VIVADO_EDITION} 	     metalfs/sdk-base:latest
	docker tag metalfs/sdk:${METAL_TAG}-${VIVADO_EDITION}-${PSL_TAG} metalfs/sdk:latest
lastest-runtime: git-is-clean all-runtime
	docker tag metalfs/runtime:${METAL_TAG}                          metalfs/runtime:latest

sdk-base:
	${docker_build} \
		-t metalfs/sdk-base:${METAL_TAG}-${VIVADO_EDITION} \
		-f sdk-base/Dockerfile \
		..

sdk:
	${docker_build} \
		-t metalfs/sdk:${METAL_TAG}-${VIVADO_EDITION}-${PSL_TAG} \
		-f sdk/Dockerfile \
		..

runtime:
	${docker_build} \
		-t metalfs/runtime:${METAL_TAG} \
		-f runtime/Dockerfile \
		..

pull-latest-sdk:
	docker pull metalfs/sdk-base:latest
	docker pull metalfs/sdk:latest
pull-latest-runtime:
	docker pull metalfs/runtime:latest

push-sdk:
	SUBTAG=$(echo ${METAL_TAG} | awk '{gsub(/\.[0-9]+$/,"",$1);print}') ; \
	while [ ! -z "$SUBTAG" ]; do \
		docker tag metalfs/sdk-base:${VIVADO_VERSION} metalfs/sdk-base:${METAL_TAG}-${VIVADO_VERSION} ; \
		docker tag metalfs/sdk:${VIVADO_VERSION}-${PSL} metalfs/sdk:${METAL_TAG}-${VIVADO_VERSION}-${PSL} ; \
		docker push metalfs/sdk-base:${METAL_TAG}-${VIVADO_VERSION} ; \
		docker push metalfs/sdk:${METAL_TAG}-${VIVADO_VERSION}-${PSL} ; \
		SUBTAG=$(echo $SUBTAG | awk '{gsub(/\.[0-9]+$/,"",$1);print}') ; \
	done
push-runtime:
	docker push metalfs/runtime:${METAL_TAG}

push-latest-sdk: git-is-clean push-sdk
	docker push metalfs/sdk-base:latest
	docker push metalfs/sdk:latest
push-latest-runtime: git-is-clean push-runtime
	docker push metalfs/runtime:latest
