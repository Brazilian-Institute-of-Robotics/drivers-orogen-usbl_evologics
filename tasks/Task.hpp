/* Generated from orogen/lib/orogen/templates/tasks/Task.hpp */

#ifndef USBL_EVOLOGICS_TASK_TASK_HPP
#define USBL_EVOLOGICS_TASK_TASK_HPP

#include "usbl_evologics/Driver.hpp"
#include "base/samples/RigidBodyState.hpp"
#include "usbl_evologics/TaskBase.hpp"
#include <queue>

namespace usbl_evologics {

    /*! \class Task 
     * \brief The task context provides and requires services. It uses an ExecutionEngine to perform its functions.
     * Essential interfaces are operations, data flow ports and properties. These interfaces have been defined using the oroGen specification.
     * In order to modify the interfaces you should (re)use oroGen and rely on the associated workflow.
     * 
     * \details
     * The name of a TaskContext is primarily defined via:
     \verbatim
     deployment 'deployment_name'
         task('custom_task_name','usbl_evologics::Task')
     end
     \endverbatim
     *  It can be dynamically adapted when the deployment is called with a prefix argument. 
     */
    class Task : public TaskBase
    {
	friend class TaskBase;
    protected:

	    boost::shared_ptr<Driver>  driver;

	    std::queue<SendIM> queueSendIM;
	    // Arbitrarily defining a max size for queueSendIM.
	    static const size_t MAX_QUEUE_MSG_SIZE = 50;
	    SendIM last_send_IM;


	    // Retries counter of instant message.
	    int im_retries_counter;
	    base::Time last_im_sent;
	    base::Time timeout_delivery_report;

	    MessageStatus message_status;

	    // Raw data counters
	    long long unsigned int sent_raw_data_counter;
	    long long unsigned int received_raw_data_counter;

	    // Manage the moment of sending new messages.
	    bool im_wait_ack;

	    AcousticConnection acoustic_connection;

	    base::Time lastStatus;

	    DeviceSettings current_settings;

	    static const size_t MAX_MSG_SIZE = 64;

        /** Reset Device to stored settings and restart it.
         *
         *  TCP connection will be closed, and device restart in DATA mode.
         */
        virtual void resetDevice(void);

        /** Clear the transmission buffer - drop raw data and instant message
         *
         */
        virtual void clearTransmissionBuffer(void);

        /** Store current settings.
         *
         */
        virtual void storeSettings(void);

        /** Restore Factory Settings and reset device.
         *
         */
        virtual void restoreFactorySettings(void);


    public:


        /** TaskContext constructor for Task
         * \param name Name of the task. This name needs to be unique to make it identifiable via nameservices.
         * \param initial_state The initial TaskState of the TaskContext. Default is Stopped state.
         */
        Task(std::string const& name = "usbl_evologics::Task");

        /** TaskContext constructor for Task 
         * \param name Name of the task. This name needs to be unique to make it identifiable for nameservices. 
         * \param engine The RTT Execution engine to be used for this task, which serialises the execution of all commands, programs, state machines and incoming events for a task. 
         * 
         */
        Task(std::string const& name, RTT::ExecutionEngine* engine);

        /** Default deconstructor of Task
         */
	~Task();

        /** This hook is called by Orocos when the state machine transitions
         * from PreOperational to Stopped. If it returns false, then the
         * component will stay in PreOperational. Otherwise, it goes into
         * Stopped.
         *
         * It is meaningful only if the #needs_configuration has been specified
         * in the task context definition with (for example):
         \verbatim
         task_context "TaskName" do
           needs_configuration
           ...
         end
         \endverbatim
         */
        bool configureHook();

        /** This hook is called by Orocos when the state machine transitions
         * from Stopped to Running. If it returns false, then the component will
         * stay in Stopped. Otherwise, it goes into Running and updateHook()
         * will be called.
         */
        bool startHook();

        /** This hook is called by Orocos when the component is in the Running
         * state, at each activity step. Here, the activity gives the "ticks"
         * when the hook should be called.
         *
         * The error(), exception() and fatal() calls, when called in this hook,
         * allow to get into the associated RunTimeError, Exception and
         * FatalError states. 
         *
         * In the first case, updateHook() is still called, and recover() allows
         * you to go back into the Running state.  In the second case, the
         * errorHook() will be called instead of updateHook(). In Exception, the
         * component is stopped and recover() needs to be called before starting
         * it again. Finally, FatalError cannot be recovered.
         */
        void updateHook();

        /** This hook is called by Orocos when the component is in the
         * RunTimeError state, at each activity step. See the discussion in
         * updateHook() about triggering options.
         *
         * Call recover() to go back in the Runtime state.
         */
        void errorHook();

        /** This hook is called by Orocos when the state machine transitions
         * from Running to Stopped after stop() has been called.
         */
        void stopHook();

        /** This hook is called by Orocos when the state machine transitions
         * from Stopped to PreOperational, requiring the call to configureHook()
         * before calling start() again.
         */
        void cleanupHook();

        /** Process I/O data
         *
         */
        void processIO();

        /** Get communication parameters
         *
         *  @return AcousticChannel with performance.
         */
        AcousticChannel getAcousticChannelparameters(void);

        /** Get settings of device
         *
         * @return DeviceSettings
         */
        DeviceSettings getDeviceSettings(void);

        /** Reset counters if it is the case.
         *
         */
        void resetCounters(bool drop_counter, bool overflow_counter);

        /** Verify if free transmission buffer is big enough to support message.
         *
         * @param buffer to be transmitted to remote device.
         * @param acoustic_connection contains amount of free buffer.
         */
        void checkFreeBuffer(std::string const &buffer, AcousticConnection const &acoustic_connection);

        /** Filter possible <+++ATcommand> in raw_data_input
         *
         *  Do not let raw_data_input mess up with connection/device.
         *  Exception if found a "+++" in raw_data_input
         *  @param raw_data_in data that goes to local device and can not have a <+++ATcommand> on it.
         */
        void filterRawData( std::string const & raw_data_in);

        /** Process notification
         *
         * Interpret Notification.
         *
         */
        void processNotification(NotificationInfo const &notification);

        /** Particular process notification
         *
         * To be implemented in subclass if particular notification is needed, like pose information (USBLLONG or USBLANGLES).
         *
         */
        virtual void processParticularNotification(NotificationInfo const &notification);

        /** Process a Report Notification
         *
         * Check if Instant Message was successfully delivered or if its failed.
         * In case of failure, act according number of retries set in device.
         * @param notification
         * @return MessageStatus to be output.
         */
        MessageStatus processDeliveryReportNotification(NotificationInfo const &notification);

        /** Get settings in string for log purpose
         *
         * @param settings of device
         * @return string with setting information
         */
        std::string getStringOfSettings(DeviceSettings settings);

        /** Get settings in string for log purpose
         *
         * @param settings of device
         * @param source_level
         * @param source_level_control
         * @return string with setting information
         */
        std::string getStringOfSettings(DeviceSettings settings, SourceLevel source_level, bool source_level_control);

        /** Dynamically update source level
         *
         * @param value, desired SourceLevel
         */
        bool setSource_level(SourceLevel value);

        /** Dynamically update source level
         *
         * @param value, True: Source level set by source level of remote device during data transmission.
         *               False: Source level set by local property.
         */
        bool setSource_level_control(bool value);

    };
}

#endif

