

#
# This makefile is intended as a starting point for building Theron with GCC, typically
# under a flavor of Linux. The comments here are intentionally brief; see the Getting
# Started section of the Theron website for more detailed information:
# http://www.theron-library.com/index.php?t=page&p=getting%20started
#
# Example command line:
#  make clean all mode=release boost=on
#
# Syntax: make <targets> <options>
#
# targets:
#   clean            deletes previous build output
#   library          builds the Theron library
#   tests            builds the test executable
#   benchmarks       builds the benchmark executables
#   tutorial         builds the sample executables
#
# options:
#   mode=debug       checked build (defines _DEBUG)
#   mode=release     optimized build (defines NDEBUG)
#   windows=[on|off] Force-enables or disables use of Windows functionality (via THERON_WINDOWS).
#   boost=[on|off]   Force-enables or disables use of Boost (via THERON_BOOST)
#   c++11=[on|off]   Force-enables or disables use of C++11 features (via THERON_CPP11)
#   posix=[on|off]   Force-enables or disables use of POSIX OS features (via THERON_POSIX)
#   numa=[on|off]    Force-enables or disables use of NUMA features (via THERON_NUMA)
#   shared=[on|off]  generates shared code (adds -fPIC to GCC command line)
#


#
# Boost and just::thread paths and library names. Change these if you need to.
# These settings are intended to match a 'typical' linux environment.
# If your environment differs, you may need to change these settings to match.
# Alternatively you may wish to copy your built or downloaded libraries to these paths.
#

BOOST_INCLUDE_PATH = /usr/include/
BOOST_RELEASE_LIB_PATH = /usr/lib/
BOOST_DEBUG_LIB_PATH = /usr/lib/debug/usr/lib/
BOOST_LIB = boost_thread


#
# Tools
# Note that this assumes you have 'rm' available, if you're using MinGW with no rm then use del instead.
#

CC = g++
AR = ar
RM = rm -f

CFLAGS = -c -Wall -pthread -D_GLIBCXX_USE_NANOSLEEP -D_GLIBCXX_USE_SCHED_YIELD
LDFLAGS = -Wl,--allow-multiple-definition -pthread
INCLUDE_FLAGS = -IInclude -IInclude/External
BUILD = Build
BIN = Bin
LIB = Lib
LIB_FLAGS =
ARFLAGS = r

#
# Use "mode=debug" or "mode=release". Default is debug.
#

ifeq ($(mode),release)
	CFLAGS += -O3 -fno-strict-aliasing -DNDEBUG
	LDFLAGS += -O3 -DNDEBUG
	LIBNAME = theron
	BOOST_LIB_PATH = ${BOOST_RELEASE_LIB_PATH}
else
	CFLAGS += -g -D_DEBUG
	LDFLAGS += -g -D_DEBUG
	LIBNAME = therond
	BOOST_LIB_PATH = ${BOOST_DEBUG_LIB_PATH}
endif

#
# Use "windows=off" to disable use of Windows features, in particular Windows threads.
# By default Windows features are auto-detected and used only if available.
#

ifeq ($(windows),off)
	CFLAGS += -DTHERON_WINDOWS=0
else ifeq ($(windows),on)
	CFLAGS += -DTHERON_WINDOWS=1
endif

#
# Use "posix=off" to disable use of POSIX features, in particular pthread.
#

ifeq ($(posix),off)
	CFLAGS += -DTHERON_POSIX=0
else ifeq ($(posix),on)
	CFLAGS += -DTHERON_POSIX=1
endif

#
# Use "boost=on" to enable use of Boost features, in particular boost::thread and Boost atomics.
# By default Boost features are assumed to be unavailable.
#

ifeq ($(boost),off)
	CFLAGS += -DTHERON_BOOST=0
else ifeq ($(boost),on)
	CFLAGS += -DTHERON_BOOST=1
	INCLUDE_FLAGS += -I${BOOST_INCLUDE_PATH} -I${BOOST_INCLUDE_PATH}
	LIB_FLAGS += -L${BOOST_LIB_PATH} -l${BOOST_LIB}
endif

#
# Use "c++11=on" to enable use of C++11 features, in particular std::thread.
# By default C++11 features are assumed to be unavailable.
#

ifeq ($(c++11),off)
	CFLAGS += -DTHERON_CPP11=0
else ifeq ($(c++11),on)
	CFLAGS += -DTHERON_CPP11=1 -std=c++11
endif

#
# Use "numa=on" to enable use of NUMA features.
# By default NUMA features are assumed to be unavailable on platforms other than Windows.
#

ifeq ($(numa),off)
	CFLAGS += -DTHERON_NUMA=0
else ifeq ($(numa),on)
	CFLAGS += -DTHERON_NUMA=1
	LIB_FLAGS += -lnuma
endif

#
# Use "shared=off" to force code generation for statically linked libraries (default).
# Use "shared=on" to force code generation for dynamically linked/shared libraries.
#

ifeq ($(shared),on)
	CFLAGS += -fPIC
endif


#
# End of user-configurable settings.
#


THERON_LIB = ${LIB}/lib${LIBNAME}.a

TESTS = ${BIN}/Tests

THREADRING = ${BIN}/ThreadRing
PARALLELTHREADRING = ${BIN}/ParallelThreadRing
PINGPONG = ${BIN}/PingPong

ALIGNMENT = ${BIN}/Alignment
CUSTOMALLOCATORS = ${BIN}/CustomAllocators
FILEREADER = ${BIN}/FileReader
HELLOWORLD = ${BIN}/HelloWorld
MESSAGEREGISTRATION = ${BIN}/MessageRegistration
UNHANDLEDMESSAGES = ${BIN}/UnhandledMessages


#
# Targets
#


all: library tests benchmarks tutorial

library: summary $(THERON_LIB)

summary:
	@echo **** Using these settings -- see makefile for options: ****
	@echo     CFLAGS = ${CFLAGS}
	@echo     LDFLAGS = ${LDFLAGS}
	@echo     LIBNAME = ${LIBNAME}
	@echo     INCLUDE_FLAGS = ${INCLUDE_FLAGS}
	@echo     LIB_FLAGS = ${LIB_FLAGS}

tests: library ${TESTS}

benchmarks: library \
	${THREADRING} \
	${PARALLELTHREADRING} \
	${PINGPONG}

tutorial: library \
	${ALIGNMENT} \
	${CUSTOMALLOCATORS} \
	${FILEREADER} \
	${HELLOWORLD} \
	${MESSAGEREGISTRATION} \
	${UNHANDLEDMESSAGES}

clean:
	${RM} ${BUILD}/*.o
	${RM} ${BUILD}/*.a
	${RM} ${BUILD}/*.ilk
	${RM} ${BIN}/*.pdb
	${RM} ${BIN}/*.ilk
	${RM} ${BIN}/*.exe
	${RM} ${LIB}/*.a
	${RM} ${LIB}/*.lib


#
# Library
#


THERON_HEADERS = \
	Include/Theron/Detail/Alignment/ActorAlignment.h \
	Include/Theron/Detail/Alignment/MessageAlignment.h \
	Include/Theron/Detail/Allocators/CachingAllocator.h \
	Include/Theron/Detail/Allocators/Pool.h \
	Include/Theron/Detail/Allocators/PoolAllocator.h \
	Include/Theron/Detail/Allocators/TrivialAllocator.h \
	Include/Theron/Detail/Allocators/ThreadsafeAllocator.h \
	Include/Theron/Detail/Containers/IntrusiveList.h \
	Include/Theron/Detail/Containers/IntrusiveQueue.h \
	Include/Theron/Detail/Containers/List.h \
	Include/Theron/Detail/Directory/Entry.h \
	Include/Theron/Detail/Directory/Directory.h \
	Include/Theron/Detail/Directory/StaticDirectory.h \
	Include/Theron/Detail/Handlers/BlindDefaultHandler.h \
	Include/Theron/Detail/Handlers/BlindFallbackHandler.h \
	Include/Theron/Detail/Handlers/DefaultFallbackHandler.h \
	Include/Theron/Detail/Handlers/DefaultHandler.h \
	Include/Theron/Detail/Handlers/DefaultHandlerCollection.h \
	Include/Theron/Detail/Handlers/FallbackHandler.h \
	Include/Theron/Detail/Handlers/FallbackHandlerCollection.h \
	Include/Theron/Detail/Handlers/HandlerCollection.h \
	Include/Theron/Detail/Handlers/IDefaultHandler.h \
	Include/Theron/Detail/Handlers/IFallbackHandler.h \
	Include/Theron/Detail/Handlers/IMessageHandler.h \
	Include/Theron/Detail/Handlers/IReceiverHandler.h \
	Include/Theron/Detail/Handlers/MessageHandler.h \
	Include/Theron/Detail/Handlers/MessageHandlerCast.h \
	Include/Theron/Detail/Handlers/ReceiverHandler.h \
    Include/Theron/Detail/Handlers/ReceiverHandlerCast.h \
	Include/Theron/Detail/Legacy/ActorRegistry.h \
	Include/Theron/Detail/Mailboxes/Mailbox.h \
	Include/Theron/Detail/Mailboxes/Queue.h \
	Include/Theron/Detail/Messages/IMessage.h \
	Include/Theron/Detail/Messages/Message.h \
	Include/Theron/Detail/Messages/MessageCast.h \
	Include/Theron/Detail/Messages/MessageCreator.h \
	Include/Theron/Detail/Messages/MessageSender.h \
	Include/Theron/Detail/Messages/MessageTraits.h \
	Include/Theron/Detail/Threading/Atomic.h \
	Include/Theron/Detail/Threading/SpinLock.h \
	Include/Theron/Detail/Threading/Thread.h \
	Include/Theron/Detail/Threading/Utils.h \
	Include/Theron/Detail/MailboxProcessor/ProcessorContext.h \
	Include/Theron/Detail/MailboxProcessor/ThreadPool.h \
	Include/Theron/Detail/MailboxProcessor/WorkerThreadStore.h \
	Include/Theron/Detail/MailboxProcessor/WorkItem.h \
	Include/Theron/Actor.h \
	Include/Theron/ActorRef.h \
	Include/Theron/Address.h \
	Include/Theron/Align.h \
	Include/Theron/AllocatorManager.h \
	Include/Theron/Assert.h \
	Include/Theron/BasicTypes.h \
	Include/Theron/Catcher.h \
	Include/Theron/Counters.h \
	Include/Theron/DefaultAllocator.h \
	Include/Theron/Defines.h \
	Include/Theron/Framework.h \
	Include/Theron/IAllocator.h \
	Include/Theron/Receiver.h \
	Include/Theron/Register.h \
	Include/Theron/Theron.h

THERON_SOURCES = \
	Theron/Actor.cpp \
	Theron/ActorRef.cpp \
	Theron/ActorRegistry.cpp \
	Theron/AllocatorManager.cpp \
	Theron/DefaultHandlerCollection.cpp \
	Theron/FallbackHandlerCollection.cpp \
	Theron/Framework.cpp \
	Theron/HandlerCollection.cpp \
	Theron/MessageSender.cpp \
	Theron/Receiver.cpp \
	Theron/WorkItem.cpp

THERON_OBJECTS = \
	${BUILD}/Actor.o \
	${BUILD}/ActorRef.o \
	${BUILD}/ActorRegistry.o \
	${BUILD}/AllocatorManager.o \
	${BUILD}/DefaultHandlerCollection.o \
	${BUILD}/FallbackHandlerCollection.o \
	${BUILD}/Framework.o \
	${BUILD}/HandlerCollection.o \
	${BUILD}/MessageSender.o \
	${BUILD}/Receiver.o \
	${BUILD}/WorkItem.o

$(THERON_LIB): $(THERON_OBJECTS)
	${AR} ${ARFLAGS} ${THERON_LIB} $(THERON_OBJECTS)

${BUILD}/Actor.o: Theron/Actor.cpp ${THERON_HEADERS}
	$(CC) $(CFLAGS) Theron/Actor.cpp -o ${BUILD}/Actor.o ${INCLUDE_FLAGS}

${BUILD}/ActorRef.o: Theron/ActorRef.cpp ${THERON_HEADERS}
	$(CC) $(CFLAGS) Theron/ActorRef.cpp -o ${BUILD}/ActorRef.o ${INCLUDE_FLAGS}

${BUILD}/ActorRegistry.o: Theron/ActorRegistry.cpp ${THERON_HEADERS}
	$(CC) $(CFLAGS) Theron/ActorRegistry.cpp -o ${BUILD}/ActorRegistry.o ${INCLUDE_FLAGS}

${BUILD}/AllocatorManager.o: Theron/AllocatorManager.cpp ${THERON_HEADERS}
	$(CC) $(CFLAGS) Theron/AllocatorManager.cpp -o ${BUILD}/AllocatorManager.o ${INCLUDE_FLAGS}

${BUILD}/DefaultHandlerCollection.o: Theron/DefaultHandlerCollection.cpp ${THERON_HEADERS}
	$(CC) $(CFLAGS) Theron/DefaultHandlerCollection.cpp -o ${BUILD}/DefaultHandlerCollection.o ${INCLUDE_FLAGS}

${BUILD}/FallbackHandlerCollection.o: Theron/FallbackHandlerCollection.cpp ${THERON_HEADERS}
	$(CC) $(CFLAGS) Theron/FallbackHandlerCollection.cpp -o ${BUILD}/FallbackHandlerCollection.o ${INCLUDE_FLAGS}

${BUILD}/Framework.o: Theron/Framework.cpp ${THERON_HEADERS}
	$(CC) $(CFLAGS) Theron/Framework.cpp -o ${BUILD}/Framework.o ${INCLUDE_FLAGS}

${BUILD}/HandlerCollection.o: Theron/HandlerCollection.cpp ${THERON_HEADERS}
	$(CC) $(CFLAGS) Theron/HandlerCollection.cpp -o ${BUILD}/HandlerCollection.o ${INCLUDE_FLAGS}

${BUILD}/MessageSender.o: Theron/MessageSender.cpp ${THERON_HEADERS}
	$(CC) $(CFLAGS) Theron/MessageSender.cpp -o ${BUILD}/MessageSender.o ${INCLUDE_FLAGS}

${BUILD}/Receiver.o: Theron/Receiver.cpp ${THERON_HEADERS}
	$(CC) $(CFLAGS) Theron/Receiver.cpp -o ${BUILD}/Receiver.o ${INCLUDE_FLAGS}

${BUILD}/WorkItem.o: Theron/WorkItem.cpp ${THERON_HEADERS}
	$(CC) $(CFLAGS) Theron/WorkItem.cpp -o ${BUILD}/WorkItem.o ${INCLUDE_FLAGS}


#
# Tests
#


TESTS_INCLUDE_FLAGS = -ITests/

TESTS_HEADERS = \
	Tests/TestFramework/ITestSuite.h \
	Tests/TestFramework/TestException.h \
	Tests/TestFramework/TestManager.h \
	Tests/TestFramework/TestSuite.h \
	Tests/TestSuites/FeatureTestSuite.h \
	Tests/TestSuites/LegacyTestSuite.h

TESTS_SOURCES = \
	Tests/Tests.cpp
	
TESTS_OBJECTS = \
	${BUILD}/Tests.o

${TESTS}: $(THERON_LIB) ${TESTS_OBJECTS}
	$(CC) $(LDFLAGS) ${TESTS_OBJECTS} $(THERON_LIB) -o ${TESTS} ${LIB_FLAGS}

${BUILD}/Tests.o: Tests/Tests.cpp ${THERON_HEADERS} ${TESTS_HEADERS}
	$(CC) $(CFLAGS) Tests/Tests.cpp -o ${BUILD}/Tests.o ${INCLUDE_FLAGS} ${TESTS_INCLUDE_FLAGS}


#
# Benchmarks
#


# ThreadRing benchmark
THREADRING_HEADERS = Benchmarks/Common/Timer.h

THREADRING_SOURCES = Benchmarks/ThreadRing/ThreadRing.cpp
THREADRING_OBJECTS = ${BUILD}/ThreadRing.o

${THREADRING}: $(THERON_LIB) ${THREADRING_OBJECTS}
	$(CC) $(LDFLAGS) ${THREADRING_OBJECTS} $(THERON_LIB) -o ${THREADRING} ${LIB_FLAGS}

${BUILD}/ThreadRing.o: Benchmarks/ThreadRing/ThreadRing.cpp ${THERON_HEADERS} ${THREADRING_HEADERS}
	$(CC) $(CFLAGS) Benchmarks/ThreadRing/ThreadRing.cpp -o ${BUILD}/ThreadRing.o ${INCLUDE_FLAGS}


# ParallelThreadRing benchmark
PARALLELTHREADRING_HEADERS = Benchmarks/Common/Timer.h

PARALLELTHREADRING_SOURCES = Benchmarks/ParallelThreadRing/ParallelThreadRing.cpp
PARALLELTHREADRING_OBJECTS = ${BUILD}/ParallelThreadRing.o

${PARALLELTHREADRING}: $(THERON_LIB) ${PARALLELTHREADRING_OBJECTS}
	$(CC) $(LDFLAGS) ${PARALLELTHREADRING_OBJECTS} $(THERON_LIB) -o ${PARALLELTHREADRING} ${LIB_FLAGS}

${BUILD}/ParallelThreadRing.o: Benchmarks/ParallelThreadRing/ParallelThreadRing.cpp ${THERON_HEADERS} ${PARALLELTHREADRING_HEADERS}
	$(CC) $(CFLAGS) Benchmarks/ParallelThreadRing/ParallelThreadRing.cpp -o ${BUILD}/ParallelThreadRing.o ${INCLUDE_FLAGS}


# PingPong benchmark
PINGPONG_SOURCES = Benchmarks/PingPong/PingPong.cpp
PINGPONG_OBJECTS = ${BUILD}/PingPong.o

${PINGPONG}: $(THERON_LIB) ${PINGPONG_OBJECTS}
	$(CC) $(LDFLAGS) ${PINGPONG_OBJECTS} $(THERON_LIB) -o ${PINGPONG} ${LIB_FLAGS}

${BUILD}/PingPong.o: Benchmarks/PingPong/PingPong.cpp ${THERON_HEADERS}
	$(CC) $(CFLAGS) Benchmarks/PingPong/PingPong.cpp -o ${BUILD}/PingPong.o ${INCLUDE_FLAGS}


#
# Tutorial
#


# Alignment sample
ALIGNMENT_HEADERS =

ALIGNMENT_SOURCES = Tutorial/Alignment/Alignment.cpp
ALIGNMENT_OBJECTS = ${BUILD}/Alignment.o

${ALIGNMENT}: $(THERON_LIB) ${ALIGNMENT_OBJECTS}
	$(CC) $(LDFLAGS) ${ALIGNMENT_OBJECTS} $(THERON_LIB) -o ${ALIGNMENT} ${LIB_FLAGS}

${BUILD}/Alignment.o: Tutorial/Alignment/Alignment.cpp ${THERON_HEADERS} ${ALIGNMENT_HEADERS}
	$(CC) $(CFLAGS) Tutorial/Alignment/Alignment.cpp -o ${BUILD}/Alignment.o ${INCLUDE_FLAGS}


# CustomAllocators sample
CUSTOMALLOCATORS_HEADERS =

CUSTOMALLOCATORS_SOURCES = Tutorial/CustomAllocators/CustomAllocators.cpp
CUSTOMALLOCATORS_OBJECTS = ${BUILD}/CustomAllocators.o

${CUSTOMALLOCATORS}: $(THERON_LIB) ${CUSTOMALLOCATORS_OBJECTS}
	$(CC) $(LDFLAGS) ${CUSTOMALLOCATORS_OBJECTS} $(THERON_LIB) -o ${CUSTOMALLOCATORS} ${LIB_FLAGS}

${BUILD}/CustomAllocators.o: Tutorial/CustomAllocators/CustomAllocators.cpp ${THERON_HEADERS} ${CUSTOMALLOCATORS_HEADERS}
	$(CC) $(CFLAGS) Tutorial/CustomAllocators/CustomAllocators.cpp -o ${BUILD}/CustomAllocators.o ${INCLUDE_FLAGS}


# FileReader sample
FILEREADER_HEADERS =

FILEREADER_SOURCES = Tutorial/FileReader/FileReader.cpp
FILEREADER_OBJECTS = ${BUILD}/FileReader.o

${FILEREADER}: $(THERON_LIB) ${FILEREADER_OBJECTS}
	$(CC) $(LDFLAGS) ${FILEREADER_OBJECTS} $(THERON_LIB) -o ${FILEREADER} ${LIB_FLAGS}

${BUILD}/FileReader.o: Tutorial/FileReader/FileReader.cpp ${THERON_HEADERS} ${FILEREADER_HEADERS}
	$(CC) $(CFLAGS) Tutorial/FileReader/FileReader.cpp -o ${BUILD}/FileReader.o ${INCLUDE_FLAGS}


# HelloWorld sample
HELLOWORLD_HEADERS =

HELLOWORLD_SOURCES = Tutorial/HelloWorld/HelloWorld.cpp
HELLOWORLD_OBJECTS = ${BUILD}/HelloWorld.o

${HELLOWORLD}: $(THERON_LIB) ${HELLOWORLD_OBJECTS}
	$(CC) $(LDFLAGS) ${HELLOWORLD_OBJECTS} $(THERON_LIB) -o ${HELLOWORLD} ${LIB_FLAGS}

${BUILD}/HelloWorld.o: Tutorial/HelloWorld/HelloWorld.cpp ${THERON_HEADERS} ${HELLOWORLD_HEADERS}
	$(CC) $(CFLAGS) Tutorial/HelloWorld/HelloWorld.cpp -o ${BUILD}/HelloWorld.o ${INCLUDE_FLAGS}


# MessageRegistration sample
MESSAGEREGISTRATION_HEADERS =

MESSAGEREGISTRATION_SOURCES = Tutorial/MessageRegistration/MessageRegistration.cpp
MESSAGEREGISTRATION_OBJECTS = ${BUILD}/MessageRegistration.o

${MESSAGEREGISTRATION}: $(THERON_LIB) ${MESSAGEREGISTRATION_OBJECTS}
	$(CC) $(LDFLAGS) ${MESSAGEREGISTRATION_OBJECTS} $(THERON_LIB) -o ${MESSAGEREGISTRATION} ${LIB_FLAGS}

${BUILD}/MessageRegistration.o: Tutorial/MessageRegistration/MessageRegistration.cpp ${THERON_HEADERS} ${MESSAGEREGISTRATION_HEADERS}
	$(CC) $(CFLAGS) Tutorial/MessageRegistration/MessageRegistration.cpp -o ${BUILD}/MessageRegistration.o ${INCLUDE_FLAGS}


# UnhandledMessages sample
UNHANDLEDMESSAGES_HEADERS =

UNHANDLEDMESSAGES_SOURCES = Tutorial/UnhandledMessages/UnhandledMessages.cpp
UNHANDLEDMESSAGES_OBJECTS = ${BUILD}/UnhandledMessages.o

${UNHANDLEDMESSAGES}: $(THERON_LIB) ${UNHANDLEDMESSAGES_OBJECTS}
	$(CC) $(LDFLAGS) ${UNHANDLEDMESSAGES_OBJECTS} $(THERON_LIB) -o ${UNHANDLEDMESSAGES} ${LIB_FLAGS}

${BUILD}/UnhandledMessages.o: Tutorial/UnhandledMessages/UnhandledMessages.cpp ${THERON_HEADERS} ${UNHANDLEDMESSAGES_HEADERS}
	$(CC) $(CFLAGS) Tutorial/UnhandledMessages/UnhandledMessages.cpp -o ${BUILD}/UnhandledMessages.o ${INCLUDE_FLAGS}

