

#
# This is the makefile for Theron, for use in building Theron with gcc.
# The other way is to use the provided VisualStudio solution Theron.sln to
# build Theron using VisualStudio under Windows. See the Getting Started
# page on the Theron website for help with using either build system.
# http://www.theron-library.com/index.php?t=page&p=getting%20started
#
# Like the solution, this makefile builds everything that there is to build in
# the Theron distribution: the theron library itself, plus the unit test and
# sample exectuables. It's set up to build with gcc. Note that the makefile has
# mainly been tested with MinGW, but usually works with minimal changes under Linux.
#
# See below for details of command line options supported by this makefile.
# In brief you can pass these command line options to make:
#
# "mode=[debug|release]". Default is debug.
# "threads=[windows|boost|std|just]". Default is boost.
# "metrics=[true|false]". Default is false.
#
# Example command line: make mode=debug threads=boost
#
# Theron can be built using four different threading implementations:
# - Windows threads (depends on windows.h)
# - Boost.Thread (depends on the Boost library)
# - standard C++ threads (requires a compliant C++11 compiler)
# - Just Thread (third-party implementation of C++ standard threads)
#
# By default, the code uses the Windows threads. However this makefile overrides
# the default, using the THERON_USE_BOOST_THREADS define to specify the use of
# Boost.Thread instead. The rationale is that if you're using the makefile you're
# likely to be on a non-Windows platform. If you'd rather use a different thread
# library then you can override the default with the threads makefile option
# on the make command line (see below).
#
# In order to build with Boost.Thread you'll need an installation of Boost.
# You can download Boost for free from http://www.boost.org - the official
# Boost website. You'll also need to build boost. The Getting Started Guide
# at http://www.boost.org/doc/libs/release/more/getting_started/index.html
# is quite useful. Note that you only need the thread and date_time components so
# if it helps you can probably build just those, using the Boost build instructions.
#
# Lastly, I should say that I'm no expert in writing makefiles, so view this
# one as a starting point! There are doubtless better ways, but hopefully it
# will be helpful in getting Theron into builds of your own projects.
# I'm kind of assuming that if you're thinking of using the makefile at all you
# already have a pretty good idea how to use it.
#

#
# Boost and just::thread paths and library names. Change these if you need to.
# These settings are intended to match a 'typical' linux-style environment.
# If your environment differs, you may need to change these settings to match.
# Alternatively you may wish to copy your built or downloaded libraries to these paths.
#

BOOST_INCLUDE_PATH = /usr/include/
BOOST_RELEASE_LIB_PATH = /usr/lib/
BOOST_DEBUG_LIB_PATH = /usr/lib/debug/usr/lib/
BOOST_RELEASE_LIB = boost_thread
BOOST_DEBUG_LIB = boost_thread

JUSTTHREAD_INCLUDE_PATH = /usr/include/justthread/
JUSTTHREAD_RELEASE_LIB_PATH = /usr/lib/
JUSTTHREAD_DEBUG_LIB_PATH = /usr/lib/
JUSTTHREAD_RELEASE_LIB = justthread3246
JUSTTHREAD_DEBUG_LIB = justthread3246

#
# Tools
# Note that this assumes you have rm installed, if you're using MinGW with no rm then use del instead.
#

CC = g++
AR = ar
RM = rm -f

CFLAGS = -c -Wall
LDFLAGS =
ARFLAGS = r

#
# Use "mode=debug" or "mode=release". Default is debug.
#

ifeq ($(mode),release)
	CFLAGS += -O3 -fno-strict-aliasing -DNDEBUG
	LDFLAGS += -O3 -DNDEBUG
	BUILD = Build
	BIN = Bin
	LIB = Lib
	LIBNAME = theron
	BOOST_LIB_PATH = ${BOOST_RELEASE_LIB_PATH}
	BOOST_LIB = ${BOOST_RELEASE_LIB}
	JUSTTHREAD_LIB_PATH = ${JUSTTHREAD_RELEASE_LIB_PATH}
	JUSTTHREAD_LIB = ${JUSTTHREAD_RELEASE_LIB}
else
	CFLAGS += -g -D_DEBUG
	LDFLAGS += -g -D_DEBUG
	BUILD = Build
	BIN = Bin
	LIB = Lib
	LIBNAME = therond
	BOOST_LIB_PATH = ${BOOST_DEBUG_LIB_PATH}
	BOOST_LIB = ${BOOST_DEBUG_LIB}
	JUSTTHREAD_LIB_PATH = ${JUSTTHREAD_DEBUG_LIB_PATH}
	JUSTTHREAD_LIB = ${JUSTTHREAD_DEBUG_LIB}
endif

#
# Use "threads=windows" to select Windows threads.
# Use "threads=boost" to select Boost.Thread (default).
# Use "threads=std" to select std::thread.
# Use "threads=just" to select just::thread.
#

ifeq ($(threads),windows)
	CFLAGS += -DTHERON_USE_BOOST_THREADS=0 -DTHERON_USE_STD_THREADS=0
	THREAD_INCLUDE_FLAGS =
	THREAD_LIB_FLAGS =
else ifeq ($(threads),std)
	CFLAGS += -DTHERON_USE_BOOST_THREADS=0 -DTHERON_USE_STD_THREADS=1 -mthreads -std=c++0x
	THREAD_INCLUDE_FLAGS =
	THREAD_LIB_FLAGS =
else ifeq ($(threads),just)
	CFLAGS += -DTHERON_USE_BOOST_THREADS=0 -DTHERON_USE_STD_THREADS=1 -mthreads -std=c++0x
	THREAD_INCLUDE_FLAGS = -I${JUSTTHREAD_INCLUDE_PATH}
	THREAD_LIB_FLAGS = -L${JUSTTHREAD_LIB_PATH} -l${JUSTTHREAD_LIB} -lwinmm
else
	CFLAGS += -DTHERON_USE_BOOST_THREADS=1 -DTHERON_USE_STD_THREADS=0 -DBOOST_THREAD_BUILD_LIB=1
	LDFLAGS += -Wl,--allow-multiple-definition -pthread
	THREAD_INCLUDE_FLAGS = -I${BOOST_INCLUDE_PATH}
	THREAD_LIB_FLAGS = -L${BOOST_LIB_PATH} -l${BOOST_LIB}
endif

#
# Use "shared=off" to force code generation for statically linked libraries (default).
# Use "shared=on" to force code generation for dynamically linked/shared libraries.
#

ifeq ($(shared),on)
	CFLAGS += -fPIC
endif

#
# End of user-defined configuration settings.
#


THERON_INCLUDE_FLAGS = -IInclude
THERON_LIB = ${LIB}/lib${LIBNAME}.a

UNITTESTS = ${BIN}/UnitTests

ACTORREFERENCES = ${BIN}/ActorReferences
CREATINGANACTOR = ${BIN}/CreatingAnActor
CUSTOMALLOCATOR = ${BIN}/CustomAllocator
DEFAULTMESSAGEHANDLER = ${BIN}/DefaultMessageHandler
DERIVEDMESSAGETYPES = ${BIN}/DerivedMessageTypes
DYNAMICHANDLERREGISTRATION = ${BIN}/DynamicHandlerRegistration
HANDLINGMESSAGES = ${BIN}/HandlingMessages
INITIALIZINGANACTOR = ${BIN}/InitializingAnActor
INITIALIZINGTHEFRAMEWORK = ${BIN}/InitializingTheFramework
REGISTERINGMESSAGES = ${BIN}/RegisteringMessages
ALIGNINGACTORS = ${BIN}/AligningActors
ALIGNINGMESSAGES = ${BIN}/AligningMessages
SENDINGMESSAGES = ${BIN}/SendingMessages
TERMINATINGTHEFRAMEWORK = ${BIN}/TerminatingTheFramework
RECEIVER = ${BIN}/Receiver
MULTIPLEFRAMEWORKS = ${BIN}/MultipleFrameworks
MEASURINGTHREADUTILIZATION = ${BIN}/MeasuringThreadUtilization
SETTINGTHETHREADCOUNT = ${BIN}/SettingTheThreadCount
NONTRIVIALMESSAGES = ${BIN}/NonTrivialMessages
ENVELOPEMESSAGES = ${BIN}/EnvelopeMessages
FILEREADER = ${BIN}/FileReader
MANAGINGTHETHREADPOOL = ${BIN}/ManagingTheThreadpool
FALLBACKHANDLER = ${BIN}/FallbackHandler
NESTEDACTORS = ${BIN}/NestedActors

THREADRING = ${BIN}/ThreadRing
PARALLELTHREADRING = ${BIN}/ParallelThreadRing
COUNTINGACTOR = ${BIN}/CountingActor
PRODUCERCONSUMER = ${BIN}/ProducerConsumer
ACTOROVERHEAD = ${BIN}/ActorOverhead
PINGPONG = ${BIN}/PingPong
MIXEDSCENARIO = ${BIN}/MixedScenario


#
# Targets
#

all: library tests samples benchmarks

library: summary $(THERON_LIB)

summary:
	@echo **** Using these settings -- see makefile for options: ****
	@echo     CFLAGS = ${CFLAGS}
	@echo     LDFLAGS = ${LDFLAGS}
	@echo     LIBNAME = ${LIBNAME}
	@echo     THREAD_INCLUDE_FLAGS = ${THREAD_INCLUDE_FLAGS}
	@echo     THREAD_LIB_FLAGS = ${THREAD_LIB_FLAGS}

tests: library ${UNITTESTS}

samples: library \
	${CREATINGANACTOR} \
	${DEFAULTMESSAGEHANDLER} \
	${DYNAMICHANDLERREGISTRATION} \
	${HANDLINGMESSAGES} \
	${RECEIVER} \
	${INITIALIZINGANACTOR} \
	${INITIALIZINGTHEFRAMEWORK} \
	${DERIVEDMESSAGETYPES} \
	${REGISTERINGMESSAGES} \
	${ALIGNINGACTORS} \
	${ALIGNINGMESSAGES} \
	${SENDINGMESSAGES} \
	${TERMINATINGTHEFRAMEWORK} \
	${ACTORREFERENCES} \
	${CUSTOMALLOCATOR} \
	${MULTIPLEFRAMEWORKS} \
	${MEASURINGTHREADUTILIZATION} \
	${SETTINGTHETHREADCOUNT} \
	${NONTRIVIALMESSAGES} \
	${ENVELOPEMESSAGES} \
	${FILEREADER} \
	${MANAGINGTHETHREADPOOL} \
	${FALLBACKHANDLER} \
	${NESTEDACTORS}

benchmarks: library \
	${THREADRING} \
	${PARALLELTHREADRING} \
	${COUNTINGACTOR} \
	${PRODUCERCONSUMER} \
	${ACTOROVERHEAD} \
	${PINGPONG} \
	${MIXEDSCENARIO}

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
# Theron library
#


THERON_HEADERS = \
	Include/Theron/Actor.h \
	Include/Theron/ActorRef.h \
	Include/Theron/Address.h \
	Include/Theron/Align.h \
	Include/Theron/AllocatorManager.h \
	Include/Theron/BasicTypes.h \
	Include/Theron/DefaultAllocator.h \
	Include/Theron/Defines.h \
	Include/Theron/Framework.h \
	Include/Theron/IAllocator.h \
	Include/Theron/Receiver.h \
	Include/Theron/Register.h \
	Include/Theron/Detail/Allocators/CachingAllocator.h \
	Include/Theron/Detail/Allocators/TrivialAllocator.h \
	Include/Theron/Detail/Allocators/Pool.h \
	Include/Theron/Detail/Containers/IntrusiveList.h \
	Include/Theron/Detail/Containers/IntrusiveQueue.h \
	Include/Theron/Detail/Containers/List.h \
	Include/Theron/Detail/Handlers/BlindDefaultHandler.h \
	Include/Theron/Detail/Handlers/DefaultHandler.h \
	Include/Theron/Detail/Handlers/IDefaultHandler.h \
	Include/Theron/Detail/Handlers/BlindFallbackHandler.h \
	Include/Theron/Detail/Handlers/DefaultFallbackHandler.h \
	Include/Theron/Detail/Handlers/FallbackHandler.h \
	Include/Theron/Detail/Handlers/IFallbackHandler.h \
	Include/Theron/Detail/Handlers/MessageHandler.h \
	Include/Theron/Detail/Handlers/IMessageHandler.h \
	Include/Theron/Detail/Handlers/MessageHandlerCast.h \
	Include/Theron/Detail/Handlers/ReceiverHandler.h \
	Include/Theron/Detail/Handlers/IReceiverHandler.h \
	Include/Theron/Detail/Handlers/ReceiverHandlerCast.h \
	Include/Theron/Detail/PagedPool/FreeList.h \
	Include/Theron/Detail/PagedPool/Page.h \
	Include/Theron/Detail/PagedPool/PagedPool.h \
	Include/Theron/Detail/Directory/ActorDirectory.h \
	Include/Theron/Detail/Directory/Directory.h \
	Include/Theron/Detail/Directory/ReceiverDirectory.h \
	Include/Theron/Detail/Messages/IMessage.h \
	Include/Theron/Detail/Messages/Message.h \
	Include/Theron/Detail/Messages/MessageAlignment.h \
	Include/Theron/Detail/Messages/MessageCast.h \
	Include/Theron/Detail/Messages/MessageCreator.h \
	Include/Theron/Detail/Messages/MessageSender.h \
	Include/Theron/Detail/Messages/MessageTraits.h \
	Include/Theron/Detail/Debug/Assert.h \
	Include/Theron/Detail/Core/ActorAlignment.h \
	Include/Theron/Detail/Core/ActorCore.h \
	Include/Theron/Detail/Core/ActorDestroyer.h \
	Include/Theron/Detail/Core/ActorCreator.h \
	Include/Theron/Detail/Core/ActorConstructor.h \
	Include/Theron/Detail/ThreadPool/ActorProcessor.h \
	Include/Theron/Detail/ThreadPool/ThreadCollection.h \
	Include/Theron/Detail/ThreadPool/ThreadPool.h \
	Include/Theron/Detail/ThreadPool/WorkerContext.h \
	Include/Theron/Detail/Threading/Lock.h \
	Include/Theron/Detail/Threading/Monitor.h \
	Include/Theron/Detail/Threading/Mutex.h \
	Include/Theron/Detail/Threading/Thread.h \
	Include/Theron/Detail/Threading/Boost/Lock.h \
	Include/Theron/Detail/Threading/Boost/Monitor.h \
	Include/Theron/Detail/Threading/Boost/Mutex.h \
	Include/Theron/Detail/Threading/Boost/Thread.h \
	Include/Theron/Detail/Threading/Std/Lock.h \
	Include/Theron/Detail/Threading/Std/Monitor.h \
	Include/Theron/Detail/Threading/Std/Mutex.h \
	Include/Theron/Detail/Threading/Std/Thread.h \
	Include/Theron/Detail/Threading/Win32/Lock.h \
	Include/Theron/Detail/Threading/Win32/Monitor.h \
	Include/Theron/Detail/Threading/Win32/Mutex.h \
	Include/Theron/Detail/Threading/Win32/Thread.h

THERON_SOURCES = \
	Source/Actor.cpp \
	Source/ActorCore.cpp \
	Source/ActorCreator.cpp \
	Source/ActorDestroyer.cpp \
	Source/ActorDirectory.cpp \
	Source/ActorRef.cpp \
	Source/Address.cpp \
	Source/AllocatorManager.cpp \
	Source/Directory.cpp \
	Source/Framework.cpp \
	Source/MessageSender.cpp \
	Source/Receiver.cpp \
	Source/ReceiverDirectory.cpp \
	Source/ThreadCollection.cpp \
	Source/ThreadPool.cpp

THERON_OBJECTS = \
	${BUILD}/Actor.o \
	${BUILD}/ActorCore.o \
	${BUILD}/ActorCreator.o \
	${BUILD}/ActorDestroyer.o \
	${BUILD}/ActorDirectory.o \
	${BUILD}/ActorRef.o \
	${BUILD}/Address.o \
	${BUILD}/AllocatorManager.o \
	${BUILD}/Directory.o \
	${BUILD}/Framework.o \
	${BUILD}/MessageSender.o \
	${BUILD}/Receiver.o \
	${BUILD}/ReceiverDirectory.o \
	${BUILD}/ThreadCollection.o \
	${BUILD}/ThreadPool.o

$(THERON_LIB): $(THERON_OBJECTS)
	${AR} ${ARFLAGS} ${THERON_LIB} $(THERON_OBJECTS)

${BUILD}/Actor.o: Source/Actor.cpp ${THERON_HEADERS}
	$(CC) $(CFLAGS) Source/Actor.cpp -o ${BUILD}/Actor.o ${THERON_INCLUDE_FLAGS} ${THREAD_INCLUDE_FLAGS}

${BUILD}/ActorCore.o: Source/ActorCore.cpp ${THERON_HEADERS}
	$(CC) $(CFLAGS) Source/ActorCore.cpp -o ${BUILD}/ActorCore.o ${THERON_INCLUDE_FLAGS} ${THREAD_INCLUDE_FLAGS}

${BUILD}/Address.o: Source/Address.cpp ${THERON_HEADERS}
	$(CC) $(CFLAGS) Source/Address.cpp -o ${BUILD}/Address.o ${THERON_INCLUDE_FLAGS} ${THREAD_INCLUDE_FLAGS}

${BUILD}/AllocatorManager.o: Source/AllocatorManager.cpp ${THERON_HEADERS}
	$(CC) $(CFLAGS) Source/AllocatorManager.cpp -o ${BUILD}/AllocatorManager.o ${THERON_INCLUDE_FLAGS} ${THREAD_INCLUDE_FLAGS}

${BUILD}/ActorDirectory.o: Source/ActorDirectory.cpp ${THERON_HEADERS}
	$(CC) $(CFLAGS) Source/ActorDirectory.cpp -o ${BUILD}/ActorDirectory.o ${THERON_INCLUDE_FLAGS} ${THREAD_INCLUDE_FLAGS}

${BUILD}/ActorRef.o: Source/ActorRef.cpp ${THERON_HEADERS}
	$(CC) $(CFLAGS) Source/ActorRef.cpp -o ${BUILD}/ActorRef.o ${THERON_INCLUDE_FLAGS} ${THREAD_INCLUDE_FLAGS}

${BUILD}/Directory.o: Source/Directory.cpp ${THERON_HEADERS}
	$(CC) $(CFLAGS) Source/Directory.cpp -o ${BUILD}/Directory.o ${THERON_INCLUDE_FLAGS} ${THREAD_INCLUDE_FLAGS}

${BUILD}/Framework.o: Source/Framework.cpp ${THERON_HEADERS}
	$(CC) $(CFLAGS) Source/Framework.cpp -o ${BUILD}/Framework.o ${THERON_INCLUDE_FLAGS} ${THREAD_INCLUDE_FLAGS}

${BUILD}/ActorCreator.o: Source/ActorCreator.cpp ${THERON_HEADERS}
	$(CC) $(CFLAGS) Source/ActorCreator.cpp -o ${BUILD}/ActorCreator.o ${THERON_INCLUDE_FLAGS} ${THREAD_INCLUDE_FLAGS}

${BUILD}/ActorDestroyer.o: Source/ActorDestroyer.cpp ${THERON_HEADERS}
	$(CC) $(CFLAGS) Source/ActorDestroyer.cpp -o ${BUILD}/ActorDestroyer.o ${THERON_INCLUDE_FLAGS} ${THREAD_INCLUDE_FLAGS}

${BUILD}/MessageSender.o: Source/MessageSender.cpp ${THERON_HEADERS}
	$(CC) $(CFLAGS) Source/MessageSender.cpp -o ${BUILD}/MessageSender.o ${THERON_INCLUDE_FLAGS} ${THREAD_INCLUDE_FLAGS}

${BUILD}/Receiver.o: Source/Receiver.cpp ${THERON_HEADERS}
	$(CC) $(CFLAGS) Source/Receiver.cpp -o ${BUILD}/Receiver.o ${THERON_INCLUDE_FLAGS} ${THREAD_INCLUDE_FLAGS}

${BUILD}/ReceiverDirectory.o: Source/ReceiverDirectory.cpp ${THERON_HEADERS}
	$(CC) $(CFLAGS) Source/ReceiverDirectory.cpp -o ${BUILD}/ReceiverDirectory.o ${THERON_INCLUDE_FLAGS} ${THREAD_INCLUDE_FLAGS}

${BUILD}/ThreadCollection.o: Source/ThreadCollection.cpp ${THERON_HEADERS}
	$(CC) $(CFLAGS) Source/ThreadCollection.cpp -o ${BUILD}/ThreadCollection.o ${THERON_INCLUDE_FLAGS} ${THREAD_INCLUDE_FLAGS}

${BUILD}/ThreadPool.o: Source/ThreadPool.cpp ${THERON_HEADERS}
	$(CC) $(CFLAGS) Source/ThreadPool.cpp -o ${BUILD}/ThreadPool.o ${THERON_INCLUDE_FLAGS} ${THREAD_INCLUDE_FLAGS}


#
# Tests
#


UNITTESTS_INCLUDE_FLAGS = -IUnitTests/

UNITTESTS_HEADERS = \
	UnitTests/TestFramework/ITestSuite.h \
	UnitTests/TestFramework/TestException.h \
	UnitTests/TestFramework/TestManager.h \
	UnitTests/TestFramework/TestSuite.h \
	UnitTests/TestSuites/ActorRefTestSuite.h \
	UnitTests/TestSuites/ActorTestSuite.h \
	UnitTests/TestSuites/DefaultAllocatorTestSuite.h \
	UnitTests/TestSuites/FrameworkTestSuite.h \
	UnitTests/TestSuites/CachingAllocatorTestSuite.h \
	UnitTests/TestSuites/ListTestSuite.h \
	UnitTests/TestSuites/MessageTestSuite.h \
	UnitTests/TestSuites/PoolTestSuite.h \
	UnitTests/TestSuites/ReceiverTestSuite.h \
	UnitTests/TestSuites/FeatureTestSuite.h \
	UnitTests/TestSuites/ThreadCollectionTestSuite.h

UNITTESTS_SOURCES = \
	UnitTests/Main.cpp \
	UnitTests/Tests.cpp
	
UNITTESTS_OBJECTS = \
	${BUILD}/UnitTests_Main.o \
	${BUILD}/UnitTests_Tests.o

${UNITTESTS}: $(THERON_LIB) ${UNITTESTS_OBJECTS}
	$(CC) $(LDFLAGS) ${UNITTESTS_OBJECTS} $(THERON_LIB) -o ${UNITTESTS} ${THREAD_LIB_FLAGS}

${BUILD}/UnitTests_Main.o: UnitTests/Main.cpp ${THERON_HEADERS} ${UNITTESTS_HEADERS}
	$(CC) $(CFLAGS) UnitTests/Main.cpp -o ${BUILD}/UnitTests_Main.o ${THERON_INCLUDE_FLAGS} ${THREAD_INCLUDE_FLAGS} ${UNITTESTS_INCLUDE_FLAGS}

${BUILD}/UnitTests_Tests.o: UnitTests/Tests.cpp ${THERON_HEADERS} ${UNITTESTS_HEADERS}
	$(CC) $(CFLAGS) UnitTests/Tests.cpp -o ${BUILD}/UnitTests_Tests.o ${THERON_INCLUDE_FLAGS} ${THREAD_INCLUDE_FLAGS} ${UNITTESTS_INCLUDE_FLAGS}



#
# Samples
#


SAMPLES_INCLUDE_FLAGS = -ISamples


# CreatingAnActor sample
CREATINGANACTOR_SOURCES = Samples/CreatingAnActor/Main.cpp
CREATINGANACTOR_OBJECTS = ${BUILD}/CreatingAnActor_Main.o

${CREATINGANACTOR}: $(THERON_LIB) ${CREATINGANACTOR_OBJECTS}
	$(CC) $(LDFLAGS) ${CREATINGANACTOR_OBJECTS} $(THERON_LIB) -o ${CREATINGANACTOR} ${THREAD_LIB_FLAGS}

${BUILD}/CreatingAnActor_Main.o: Samples/CreatingAnActor/Main.cpp ${THERON_HEADERS}
	$(CC) $(CFLAGS) Samples/CreatingAnActor/Main.cpp -o ${BUILD}/CreatingAnActor_Main.o ${THERON_INCLUDE_FLAGS} ${THREAD_INCLUDE_FLAGS} ${SAMPLES_INCLUDE_FLAGS}
	

# DefaultMessageHandler sample
DEFAULTMESSAGEHANDLER_SOURCES = Samples/DefaultMessageHandler/Main.cpp
DEFAULTMESSAGEHANDLER_OBJECTS = ${BUILD}/DefaultMessageHandler_Main.o

${DEFAULTMESSAGEHANDLER}: $(THERON_LIB) ${DEFAULTMESSAGEHANDLER_OBJECTS}
	$(CC) $(LDFLAGS) ${DEFAULTMESSAGEHANDLER_OBJECTS} $(THERON_LIB) -o ${DEFAULTMESSAGEHANDLER} ${THREAD_LIB_FLAGS}

${BUILD}/DefaultMessageHandler_Main.o: Samples/DefaultMessageHandler/Main.cpp ${THERON_HEADERS}
	$(CC) $(CFLAGS) Samples/DefaultMessageHandler/Main.cpp -o ${BUILD}/DefaultMessageHandler_Main.o ${THERON_INCLUDE_FLAGS} ${THREAD_INCLUDE_FLAGS} ${SAMPLES_INCLUDE_FLAGS}
	

# DynamicHandlerRegistration sample
DYNAMICHANDLERREGISTRATION_SOURCES = Samples/DynamicHandlerRegistration/Main.cpp
DYNAMICHANDLERREGISTRATION_OBJECTS = ${BUILD}/DynamicHandlerRegistration_Main.o

${DYNAMICHANDLERREGISTRATION}: $(THERON_LIB) ${DYNAMICHANDLERREGISTRATION_OBJECTS}
	$(CC) $(LDFLAGS) ${DYNAMICHANDLERREGISTRATION_OBJECTS} $(THERON_LIB) -o ${DYNAMICHANDLERREGISTRATION} ${THREAD_LIB_FLAGS}

${BUILD}/DynamicHandlerRegistration_Main.o: Samples/DynamicHandlerRegistration/Main.cpp ${THERON_HEADERS}
	$(CC) $(CFLAGS) Samples/DynamicHandlerRegistration/Main.cpp -o ${BUILD}/DynamicHandlerRegistration_Main.o ${THERON_INCLUDE_FLAGS} ${THREAD_INCLUDE_FLAGS} ${SAMPLES_INCLUDE_FLAGS}


# HandlingMessages sample
HANDLINGMESSAGES_SOURCES = Samples/HandlingMessages/Main.cpp
HANDLINGMESSAGES_OBJECTS = ${BUILD}/HandlingMessages_Main.o

${HANDLINGMESSAGES}: $(THERON_LIB) ${HANDLINGMESSAGES_OBJECTS}
	$(CC) $(LDFLAGS) ${HANDLINGMESSAGES_OBJECTS} $(THERON_LIB) -o ${HANDLINGMESSAGES} ${THREAD_LIB_FLAGS}

${BUILD}/HandlingMessages_Main.o: Samples/HandlingMessages/Main.cpp ${THERON_HEADERS}
	$(CC) $(CFLAGS) Samples/HandlingMessages/Main.cpp -o ${BUILD}/HandlingMessages_Main.o ${THERON_INCLUDE_FLAGS} ${THREAD_INCLUDE_FLAGS} ${SAMPLES_INCLUDE_FLAGS}
	

# Receiver sample
RECEIVER_SOURCES = Samples/Receiver/Main.cpp
RECEIVER_OBJECTS = ${BUILD}/Receiver_Main.o

${RECEIVER}: $(THERON_LIB) ${RECEIVER_OBJECTS}
	$(CC) $(LDFLAGS) ${RECEIVER_OBJECTS} $(THERON_LIB) -o ${RECEIVER} ${THREAD_LIB_FLAGS}

${BUILD}/Receiver_Main.o: Samples/Receiver/Main.cpp ${THERON_HEADERS}
	$(CC) $(CFLAGS) Samples/Receiver/Main.cpp -o ${BUILD}/Receiver_Main.o ${THERON_INCLUDE_FLAGS} ${THREAD_INCLUDE_FLAGS} ${SAMPLES_INCLUDE_FLAGS}
	

# InitializingAnActor sample
INITIALIZINGANACTOR_SOURCES = Samples/InitializingAnActor/Main.cpp
INITIALIZINGANACTOR_OBJECTS = ${BUILD}/InitializingAnActor_Main.o

${INITIALIZINGANACTOR}: $(THERON_LIB) ${INITIALIZINGANACTOR_OBJECTS}
	$(CC) $(LDFLAGS) ${INITIALIZINGANACTOR_OBJECTS} $(THERON_LIB) -o ${INITIALIZINGANACTOR} ${THREAD_LIB_FLAGS}

${BUILD}/InitializingAnActor_Main.o: Samples/InitializingAnActor/Main.cpp ${THERON_HEADERS}
	$(CC) $(CFLAGS) Samples/InitializingAnActor/Main.cpp -o ${BUILD}/InitializingAnActor_Main.o ${THERON_INCLUDE_FLAGS} ${THREAD_INCLUDE_FLAGS} ${SAMPLES_INCLUDE_FLAGS}
	

# InitializingTheFramework sample
INITIALIZINGTHEFRAMEWORK_SOURCES = Samples/InitializingTheFramework/Main.cpp
INITIALIZINGTHEFRAMEWORK_OBJECTS = ${BUILD}/InitializingTheFramework_Main.o

${INITIALIZINGTHEFRAMEWORK}: $(THERON_LIB) ${INITIALIZINGTHEFRAMEWORK_OBJECTS}
	$(CC) $(LDFLAGS) ${INITIALIZINGTHEFRAMEWORK_OBJECTS} $(THERON_LIB) -o ${INITIALIZINGTHEFRAMEWORK} ${THREAD_LIB_FLAGS}

${BUILD}/InitializingTheFramework_Main.o: Samples/InitializingTheFramework/Main.cpp ${THERON_HEADERS}
	$(CC) $(CFLAGS) Samples/InitializingTheFramework/Main.cpp -o ${BUILD}/InitializingTheFramework_Main.o ${THERON_INCLUDE_FLAGS} ${THREAD_INCLUDE_FLAGS} ${SAMPLES_INCLUDE_FLAGS}
	

# DerivedMessageTypes sample
DERIVEDMESSAGETYPES_SOURCES = Samples/DerivedMessageTypes/Main.cpp
DERIVEDMESSAGETYPES_OBJECTS = ${BUILD}/DerivedMessageTypes_Main.o

${DERIVEDMESSAGETYPES}: $(THERON_LIB) ${DERIVEDMESSAGETYPES_OBJECTS}
	$(CC) $(LDFLAGS) ${DERIVEDMESSAGETYPES_OBJECTS} $(THERON_LIB) -o ${DERIVEDMESSAGETYPES} ${THREAD_LIB_FLAGS}

${BUILD}/DerivedMessageTypes_Main.o: Samples/DerivedMessageTypes/Main.cpp ${THERON_HEADERS}
	$(CC) $(CFLAGS) Samples/DerivedMessageTypes/Main.cpp -o ${BUILD}/DerivedMessageTypes_Main.o ${THERON_INCLUDE_FLAGS} ${THREAD_INCLUDE_FLAGS} ${SAMPLES_INCLUDE_FLAGS}
	

# RegisteringMessages sample
REGISTERINGMESSAGES_SOURCES = Samples/RegisteringMessages/Main.cpp
REGISTERINGMESSAGES_OBJECTS = ${BUILD}/RegisteringMessages_Main.o

${REGISTERINGMESSAGES}: $(THERON_LIB) ${REGISTERINGMESSAGES_OBJECTS}
	$(CC) $(LDFLAGS) ${REGISTERINGMESSAGES_OBJECTS} $(THERON_LIB) -o ${REGISTERINGMESSAGES} ${THREAD_LIB_FLAGS} -fno-rtti

${BUILD}/RegisteringMessages_Main.o: Samples/RegisteringMessages/Main.cpp ${THERON_HEADERS}
	$(CC) $(CFLAGS) Samples/RegisteringMessages/Main.cpp -o ${BUILD}/RegisteringMessages_Main.o ${THERON_INCLUDE_FLAGS} ${THREAD_INCLUDE_FLAGS} ${SAMPLES_INCLUDE_FLAGS}


# AligningActors sample
ALIGNINGACTORS_HEADERS = Samples/Common/LinearAllocator.h
ALIGNINGACTORS_SOURCES = Samples/AligningActors/Main.cpp
ALIGNINGACTORS_OBJECTS = ${BUILD}/AligningActors_Main.o

${ALIGNINGACTORS}: $(THERON_LIB) ${ALIGNINGACTORS_OBJECTS}
	$(CC) $(LDFLAGS) ${ALIGNINGACTORS_OBJECTS} $(THERON_LIB) -o ${ALIGNINGACTORS} ${THREAD_LIB_FLAGS}

${BUILD}/AligningActors_Main.o: Samples/AligningActors/Main.cpp ${THERON_HEADERS} ${ALIGNINGACTORS_HEADERS}
	$(CC) $(CFLAGS) Samples/AligningActors/Main.cpp -o ${BUILD}/AligningActors_Main.o ${THERON_INCLUDE_FLAGS} ${THREAD_INCLUDE_FLAGS} ${SAMPLES_INCLUDE_FLAGS}


# AligningMessages sample
ALIGNINGMESSAGES_HEADERS = Samples/Common/LinearAllocator.h
ALIGNINGMESSAGES_SOURCES = Samples/AligningMessages/Main.cpp
ALIGNINGMESSAGES_OBJECTS = ${BUILD}/AligningMessages_Main.o

${ALIGNINGMESSAGES}: $(THERON_LIB) ${ALIGNINGMESSAGES_OBJECTS}
	$(CC) $(LDFLAGS) ${ALIGNINGMESSAGES_OBJECTS} $(THERON_LIB) -o ${ALIGNINGMESSAGES} ${THREAD_LIB_FLAGS}

${BUILD}/AligningMessages_Main.o: Samples/AligningMessages/Main.cpp ${THERON_HEADERS} ${ALIGNINGMESSAGES_HEADERS}
	$(CC) $(CFLAGS) Samples/AligningMessages/Main.cpp -o ${BUILD}/AligningMessages_Main.o ${THERON_INCLUDE_FLAGS} ${THREAD_INCLUDE_FLAGS} ${SAMPLES_INCLUDE_FLAGS}


# SendingMessages sample
SENDINGMESSAGES_SOURCES = Samples/SendingMessages/Main.cpp
SENDINGMESSAGES_OBJECTS = ${BUILD}/SendingMessages_Main.o

${SENDINGMESSAGES}: $(THERON_LIB) ${SENDINGMESSAGES_OBJECTS}
	$(CC) $(LDFLAGS) ${SENDINGMESSAGES_OBJECTS} $(THERON_LIB) -o ${SENDINGMESSAGES} ${THREAD_LIB_FLAGS}

${BUILD}/SendingMessages_Main.o: Samples/SendingMessages/Main.cpp ${THERON_HEADERS}
	$(CC) $(CFLAGS) Samples/SendingMessages/Main.cpp -o ${BUILD}/SendingMessages_Main.o ${THERON_INCLUDE_FLAGS} ${THREAD_INCLUDE_FLAGS} ${SAMPLES_INCLUDE_FLAGS}


# TerminatingTheFramework sample
TERMINATINGTHEFRAMEWORK_SOURCES = Samples/TerminatingTheFramework/Main.cpp
TERMINATINGTHEFRAMEWORK_OBJECTS = ${BUILD}/TerminatingTheFramework_Main.o

${TERMINATINGTHEFRAMEWORK}: $(THERON_LIB) ${TERMINATINGTHEFRAMEWORK_OBJECTS}
	$(CC) $(LDFLAGS) ${TERMINATINGTHEFRAMEWORK_OBJECTS} $(THERON_LIB) -o ${TERMINATINGTHEFRAMEWORK} ${THREAD_LIB_FLAGS}

${BUILD}/TerminatingTheFramework_Main.o: Samples/TerminatingTheFramework/Main.cpp ${THERON_HEADERS}
	$(CC) $(CFLAGS) Samples/TerminatingTheFramework/Main.cpp -o ${BUILD}/TerminatingTheFramework_Main.o ${THERON_INCLUDE_FLAGS} ${THREAD_INCLUDE_FLAGS} ${SAMPLES_INCLUDE_FLAGS}
	

# ActorReferences sample
ACTORREFERENCES_SOURCES = Samples/ActorReferences/Main.cpp
ACTORREFERENCES_OBJECTS = ${BUILD}/ActorReferences_Main.o

${ACTORREFERENCES}: $(THERON_LIB) ${ACTORREFERENCES_OBJECTS}
	$(CC) $(LDFLAGS) ${ACTORREFERENCES_OBJECTS} $(THERON_LIB) -o ${ACTORREFERENCES} ${THREAD_LIB_FLAGS}

${BUILD}/ActorReferences_Main.o: Samples/ActorReferences/Main.cpp ${THERON_HEADERS}
	$(CC) $(CFLAGS) Samples/ActorReferences/Main.cpp -o ${BUILD}/ActorReferences_Main.o ${THERON_INCLUDE_FLAGS} ${THREAD_INCLUDE_FLAGS} ${SAMPLES_INCLUDE_FLAGS}
	

# CustomAllocator sample
CUSTOMALLOCATOR_HEADERS = Samples/Common/LinearAllocator.h
CUSTOMALLOCATOR_SOURCES = Samples/CustomAllocator/Main.cpp
CUSTOMALLOCATOR_OBJECTS = ${BUILD}/CustomAllocator_Main.o

${CUSTOMALLOCATOR}: $(THERON_LIB) ${CUSTOMALLOCATOR_OBJECTS}
	$(CC) $(LDFLAGS) ${CUSTOMALLOCATOR_OBJECTS} $(THERON_LIB) -o ${CUSTOMALLOCATOR} ${THREAD_LIB_FLAGS}

${BUILD}/CustomAllocator_Main.o: Samples/CustomAllocator/Main.cpp ${THERON_HEADERS} ${CUSTOMALLOCATOR_HEADERS}
	$(CC) $(CFLAGS) Samples/CustomAllocator/Main.cpp -o ${BUILD}/CustomAllocator_Main.o ${THERON_INCLUDE_FLAGS} ${THREAD_INCLUDE_FLAGS} ${SAMPLES_INCLUDE_FLAGS}


# MultipleFrameworks sample
MULTIPLEFRAMEWORKS_SOURCES = Samples/MultipleFrameworks/Main.cpp
MULTIPLEFRAMEWORKS_OBJECTS = ${BUILD}/MultipleFrameworks_Main.o

${MULTIPLEFRAMEWORKS}: $(THERON_LIB) ${MULTIPLEFRAMEWORKS_OBJECTS}
	$(CC) $(LDFLAGS) ${MULTIPLEFRAMEWORKS_OBJECTS} $(THERON_LIB) -o ${MULTIPLEFRAMEWORKS} ${THREAD_LIB_FLAGS}

${BUILD}/MultipleFrameworks_Main.o: Samples/MultipleFrameworks/Main.cpp ${THERON_HEADERS}
	$(CC) $(CFLAGS) Samples/MultipleFrameworks/Main.cpp -o ${BUILD}/MultipleFrameworks_Main.o ${THERON_INCLUDE_FLAGS} ${THREAD_INCLUDE_FLAGS} ${SAMPLES_INCLUDE_FLAGS}


# MeasuringThreadUtilization sample
MEASURINGTHREADUTILIZATION_SOURCES = Samples/MeasuringThreadUtilization/Main.cpp
MEASURINGTHREADUTILIZATION_OBJECTS = ${BUILD}/MeasuringThreadUtilization_Main.o

${MEASURINGTHREADUTILIZATION}: $(THERON_LIB) ${MEASURINGTHREADUTILIZATION_OBJECTS}
	$(CC) $(LDFLAGS) ${MEASURINGTHREADUTILIZATION_OBJECTS} $(THERON_LIB) -o ${MEASURINGTHREADUTILIZATION} ${THREAD_LIB_FLAGS}

${BUILD}/MeasuringThreadUtilization_Main.o: Samples/MeasuringThreadUtilization/Main.cpp ${THERON_HEADERS}
	$(CC) $(CFLAGS) Samples/MeasuringThreadUtilization/Main.cpp -o ${BUILD}/MeasuringThreadUtilization_Main.o ${THERON_INCLUDE_FLAGS} ${THREAD_INCLUDE_FLAGS} ${SAMPLES_INCLUDE_FLAGS}


# SettingTheThreadCount sample
SETTINGTHETHREADCOUNT_SOURCES = Samples/SettingTheThreadCount/Main.cpp
SETTINGTHETHREADCOUNT_OBJECTS = ${BUILD}/SettingTheThreadCount_Main.o

${SETTINGTHETHREADCOUNT}: $(THERON_LIB) ${SETTINGTHETHREADCOUNT_OBJECTS}
	$(CC) $(LDFLAGS) ${SETTINGTHETHREADCOUNT_OBJECTS} $(THERON_LIB) -o ${SETTINGTHETHREADCOUNT} ${THREAD_LIB_FLAGS}

${BUILD}/SettingTheThreadCount_Main.o: Samples/SettingTheThreadCount/Main.cpp ${THERON_HEADERS}
	$(CC) $(CFLAGS) Samples/SettingTheThreadCount/Main.cpp -o ${BUILD}/SettingTheThreadCount_Main.o ${THERON_INCLUDE_FLAGS} ${THREAD_INCLUDE_FLAGS} ${SAMPLES_INCLUDE_FLAGS}


# NonTrivialMessages sample
NONTRIVIALMESSAGES_SOURCES = Samples/NonTrivialMessages/Main.cpp
NONTRIVIALMESSAGES_OBJECTS = ${BUILD}/NonTrivialMessages_Main.o

${NONTRIVIALMESSAGES}: $(THERON_LIB) ${NONTRIVIALMESSAGES_OBJECTS}
	$(CC) $(LDFLAGS) ${NONTRIVIALMESSAGES_OBJECTS} $(THERON_LIB) -o ${NONTRIVIALMESSAGES} ${THREAD_LIB_FLAGS}

${BUILD}/NonTrivialMessages_Main.o: Samples/NonTrivialMessages/Main.cpp ${THERON_HEADERS}
	$(CC) $(CFLAGS) Samples/NonTrivialMessages/Main.cpp -o ${BUILD}/NonTrivialMessages_Main.o ${THERON_INCLUDE_FLAGS} ${THREAD_INCLUDE_FLAGS} ${SAMPLES_INCLUDE_FLAGS}


# EnvelopeMessages sample
ENVELOPEMESSAGES_SOURCES = Samples/EnvelopeMessages/Main.cpp
ENVELOPEMESSAGES_OBJECTS = ${BUILD}/EnvelopeMessages_Main.o

${ENVELOPEMESSAGES}: $(THERON_LIB) ${ENVELOPEMESSAGES_OBJECTS}
	$(CC) $(LDFLAGS) ${ENVELOPEMESSAGES_OBJECTS} $(THERON_LIB) -o ${ENVELOPEMESSAGES} ${THREAD_LIB_FLAGS}

${BUILD}/EnvelopeMessages_Main.o: Samples/EnvelopeMessages/Main.cpp ${THERON_HEADERS}
	$(CC) $(CFLAGS) Samples/EnvelopeMessages/Main.cpp -o ${BUILD}/EnvelopeMessages_Main.o ${THERON_INCLUDE_FLAGS} ${THREAD_INCLUDE_FLAGS} ${SAMPLES_INCLUDE_FLAGS}


# FileReader sample
FILEREADER_SOURCES = Samples/FileReader/Main.cpp
FILEREADER_OBJECTS = ${BUILD}/FileReader_Main.o

${FILEREADER}: $(THERON_LIB) ${FILEREADER_OBJECTS}
	$(CC) $(LDFLAGS) ${FILEREADER_OBJECTS} $(THERON_LIB) -o ${FILEREADER} ${THREAD_LIB_FLAGS}

${BUILD}/FileReader_Main.o: Samples/FileReader/Main.cpp ${THERON_HEADERS}
	$(CC) $(CFLAGS) Samples/FileReader/Main.cpp -o ${BUILD}/FileReader_Main.o ${THERON_INCLUDE_FLAGS} ${THREAD_INCLUDE_FLAGS} ${SAMPLES_INCLUDE_FLAGS}
	

# ManagingTheThreadpool sample
MANAGINGTHETHREADPOOL_SOURCES = Samples/ManagingTheThreadpool/Main.cpp
MANAGINGTHETHREADPOOL_OBJECTS = ${BUILD}/ManagingTheThreadpool_Main.o

${MANAGINGTHETHREADPOOL}: $(THERON_LIB) ${MANAGINGTHETHREADPOOL_OBJECTS}
	$(CC) $(LDFLAGS) ${MANAGINGTHETHREADPOOL_OBJECTS} $(THERON_LIB) -o ${MANAGINGTHETHREADPOOL} ${THREAD_LIB_FLAGS}

${BUILD}/ManagingTheThreadpool_Main.o: Samples/ManagingTheThreadpool/Main.cpp ${THERON_HEADERS}
	$(CC) $(CFLAGS) Samples/ManagingTheThreadpool/Main.cpp -o ${BUILD}/ManagingTheThreadpool_Main.o ${THERON_INCLUDE_FLAGS} ${THREAD_INCLUDE_FLAGS} ${SAMPLES_INCLUDE_FLAGS}


# FallbackHandler sample
FALLBACKHANDLER_SOURCES = Samples/FallbackHandler/Main.cpp
FALLBACKHANDLER_OBJECTS = ${BUILD}/FallbackHandler_Main.o

${FALLBACKHANDLER}: $(THERON_LIB) ${FALLBACKHANDLER_OBJECTS}
	$(CC) $(LDFLAGS) ${FALLBACKHANDLER_OBJECTS} $(THERON_LIB) -o ${FALLBACKHANDLER} ${THREAD_LIB_FLAGS}

${BUILD}/FallbackHandler_Main.o: Samples/FallbackHandler/Main.cpp ${THERON_HEADERS}
	$(CC) $(CFLAGS) Samples/FallbackHandler/Main.cpp -o ${BUILD}/FallbackHandler_Main.o ${THERON_INCLUDE_FLAGS} ${THREAD_INCLUDE_FLAGS} ${SAMPLES_INCLUDE_FLAGS}


# NestedActors sample
NESTEDACTORS_SOURCES = Samples/NestedActors/Main.cpp
NESTEDACTORS_OBJECTS = ${BUILD}/NestedActors_Main.o

${NESTEDACTORS}: $(THERON_LIB) ${NESTEDACTORS_OBJECTS}
	$(CC) $(LDFLAGS) ${NESTEDACTORS_OBJECTS} $(THERON_LIB) -o ${NESTEDACTORS} ${THREAD_LIB_FLAGS}

${BUILD}/NestedActors_Main.o: Samples/NestedActors/Main.cpp ${THERON_HEADERS}
	$(CC) $(CFLAGS) Samples/NestedActors/Main.cpp -o ${BUILD}/NestedActors_Main.o ${THERON_INCLUDE_FLAGS} ${THREAD_INCLUDE_FLAGS} ${SAMPLES_INCLUDE_FLAGS}


#
# Benchmarks
#


# ThreadRing benchmark
THREADRING_HEADERS = Benchmarks/Common/Timer.h

THREADRING_SOURCES = Benchmarks/ThreadRing/Main.cpp
THREADRING_OBJECTS = ${BUILD}/ThreadRing_Main.o

${THREADRING}: $(THERON_LIB) ${THREADRING_OBJECTS}
	$(CC) $(LDFLAGS) ${THREADRING_OBJECTS} $(THERON_LIB) -o ${THREADRING} ${THREAD_LIB_FLAGS}

${BUILD}/ThreadRing_Main.o: Benchmarks/ThreadRing/Main.cpp ${THERON_HEADERS} ${THREADRING_HEADERS}
	$(CC) $(CFLAGS) Benchmarks/ThreadRing/Main.cpp -o ${BUILD}/ThreadRing_Main.o ${THERON_INCLUDE_FLAGS} ${THREAD_INCLUDE_FLAGS}


# ParallelThreadRing benchmark
PARALLELTHREADRING_HEADERS = Benchmarks/Common/Timer.h

PARALLELTHREADRING_SOURCES = Benchmarks/ParallelThreadRing/Main.cpp
PARALLELTHREADRING_OBJECTS = ${BUILD}/ParallelThreadRing_Main.o

${PARALLELTHREADRING}: $(THERON_LIB) ${PARALLELTHREADRING_OBJECTS}
	$(CC) $(LDFLAGS) ${PARALLELTHREADRING_OBJECTS} $(THERON_LIB) -o ${PARALLELTHREADRING} ${THREAD_LIB_FLAGS}

${BUILD}/ParallelThreadRing_Main.o: Benchmarks/ParallelThreadRing/Main.cpp ${THERON_HEADERS} ${PARALLELTHREADRING_HEADERS}
	$(CC) $(CFLAGS) Benchmarks/ParallelThreadRing/Main.cpp -o ${BUILD}/ParallelThreadRing_Main.o ${THERON_INCLUDE_FLAGS} ${THREAD_INCLUDE_FLAGS}


# CountingActor benchmark
COUNTINGACTOR_HEADERS = Benchmarks/Common/Timer.h

COUNTINGACTOR_SOURCES = Benchmarks/CountingActor/Main.cpp
COUNTINGACTOR_OBJECTS = ${BUILD}/CountingActor_Main.o

${COUNTINGACTOR}: $(THERON_LIB) ${COUNTINGACTOR_OBJECTS}
	$(CC) $(LDFLAGS) ${COUNTINGACTOR_OBJECTS} $(THERON_LIB) -o ${COUNTINGACTOR} ${THREAD_LIB_FLAGS}

${BUILD}/CountingActor_Main.o: Benchmarks/CountingActor/Main.cpp ${THERON_HEADERS} ${COUNTINGACTOR_HEADERS}
	$(CC) $(CFLAGS) Benchmarks/CountingActor/Main.cpp -o ${BUILD}/CountingActor_Main.o ${THERON_INCLUDE_FLAGS} ${THREAD_INCLUDE_FLAGS}


# ProducerConsumer benchmark
PRODUCERCONSUMER_HEADERS = Benchmarks/Common/Timer.h

PRODUCERCONSUMER_SOURCES = Benchmarks/ProducerConsumer/Main.cpp
PRODUCERCONSUMER_OBJECTS = ${BUILD}/ProducerConsumer_Main.o

${PRODUCERCONSUMER}: $(THERON_LIB) ${PRODUCERCONSUMER_OBJECTS}
	$(CC) $(LDFLAGS) ${PRODUCERCONSUMER_OBJECTS} $(THERON_LIB) -o ${PRODUCERCONSUMER} ${THREAD_LIB_FLAGS}

${BUILD}/ProducerConsumer_Main.o: Benchmarks/ProducerConsumer/Main.cpp ${THERON_HEADERS} ${PRODUCERCONSUMER_HEADERS}
	$(CC) $(CFLAGS) Benchmarks/ProducerConsumer/Main.cpp -o ${BUILD}/ProducerConsumer_Main.o ${THERON_INCLUDE_FLAGS} ${THREAD_INCLUDE_FLAGS}


# ActorOverhead benchmark
ACTOROVERHEAD_SOURCES = Benchmarks/ActorOverhead/Main.cpp
ACTOROVERHEAD_OBJECTS = ${BUILD}/ActorOverhead_Main.o

${ACTOROVERHEAD}: $(THERON_LIB) ${ACTOROVERHEAD_OBJECTS}
	$(CC) $(LDFLAGS) ${ACTOROVERHEAD_OBJECTS} $(THERON_LIB) -o ${ACTOROVERHEAD} ${THREAD_LIB_FLAGS}

${BUILD}/ActorOverhead_Main.o: Benchmarks/ActorOverhead/Main.cpp ${THERON_HEADERS}
	$(CC) $(CFLAGS) Benchmarks/ActorOverhead/Main.cpp -o ${BUILD}/ActorOverhead_Main.o ${THERON_INCLUDE_FLAGS} ${THREAD_INCLUDE_FLAGS}


# PingPong benchmark
PINGPONG_SOURCES = Benchmarks/PingPong/Main.cpp
PINGPONG_OBJECTS = ${BUILD}/PingPong_Main.o

${PINGPONG}: $(THERON_LIB) ${PINGPONG_OBJECTS}
	$(CC) $(LDFLAGS) ${PINGPONG_OBJECTS} $(THERON_LIB) -o ${PINGPONG} ${THREAD_LIB_FLAGS}

${BUILD}/PingPong_Main.o: Benchmarks/PingPong/Main.cpp ${THERON_HEADERS}
	$(CC) $(CFLAGS) Benchmarks/PingPong/Main.cpp -o ${BUILD}/PingPong_Main.o ${THERON_INCLUDE_FLAGS} ${THREAD_INCLUDE_FLAGS}


# MixedScenario benchmark
MIXEDSCENARIO_SOURCES = Benchmarks/MixedScenario/Main.cpp
MIXEDSCENARIO_OBJECTS = ${BUILD}/MixedScenario_Main.o

${MIXEDSCENARIO}: $(THERON_LIB) ${MIXEDSCENARIO_OBJECTS}
	$(CC) $(LDFLAGS) ${MIXEDSCENARIO_OBJECTS} $(THERON_LIB) -o ${MIXEDSCENARIO} ${THREAD_LIB_FLAGS}

${BUILD}/MixedScenario_Main.o: Benchmarks/MixedScenario/Main.cpp ${THERON_HEADERS}
	$(CC) $(CFLAGS) Benchmarks/MixedScenario/Main.cpp -o ${BUILD}/MixedScenario_Main.o ${THERON_INCLUDE_FLAGS} ${THREAD_INCLUDE_FLAGS}


