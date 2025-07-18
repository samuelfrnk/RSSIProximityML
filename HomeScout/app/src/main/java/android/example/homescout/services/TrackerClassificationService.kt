package android.example.homescout.services

import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.NotificationManager.IMPORTANCE_LOW
import android.app.PendingIntent
import android.app.PendingIntent.FLAG_IMMUTABLE
import android.content.Context
import android.content.Intent
import android.example.homescout.R
import android.example.homescout.database.BLEDevice
import android.example.homescout.database.MaliciousTracker
import android.example.homescout.repositories.MainRepository
import android.example.homescout.repositories.TrackingPreferencesRepository
import android.example.homescout.ui.main.MainActivity
import android.example.homescout.utils.Constants
import android.example.homescout.utils.Constants.ACTION_SHOW_NOTIFICATIONS_FRAGMENT
import android.example.homescout.utils.Constants.ACTION_SHOW_SETTINGS_FRAGMENT
import android.example.homescout.utils.Constants.ACTION_START_TRACKER_CLASSIFICATION_SERVICE
import android.example.homescout.utils.Constants.ACTION_STOP_TRACKER_CLASSIFICATION_SERVICE
import android.example.homescout.utils.Constants.CHANNEL_ID_TRACKER_CLASSIFICATION
import android.example.homescout.utils.Constants.INTERVAL_TRACKER_CLASSIFICATION
import android.example.homescout.utils.Constants.NOTIFICATION_CHANNEL_TRACKER_CLASSIFICATION
import android.example.homescout.utils.Constants.NOTIFICATION_ID_TRACKER_CLASSIFICATION
import android.location.Location
import android.os.Handler
import android.os.Looper
import androidx.core.app.NotificationCompat
import androidx.lifecycle.LifecycleService
import androidx.lifecycle.asLiveData
import androidx.lifecycle.lifecycleScope
import dagger.hilt.android.AndroidEntryPoint
import kotlinx.coroutines.launch
import timber.log.Timber
import javax.inject.Inject
import ai.onnxruntime.*
import android.example.homescout.models.AirTag
import androidx.lifecycle.observe
import java.util.Collections


@AndroidEntryPoint
class TrackerClassificationService : LifecycleService() {


    private val handler = Handler(Looper.getMainLooper())

    private var isServiceRunning = false

    private var distance : Float? = null
    private var timeinMin : Float? = null
    private var occurrences : Float? = null
    private var isRssiShield: Boolean ?= null
    private var isIndoor: Boolean ?= null
    private var isLos: Boolean ?= null

    private lateinit var ortEnvironment: OrtEnvironment
    private lateinit var ortSession: OrtSession

    @Inject
    lateinit var trackingPreferencesRepository: TrackingPreferencesRepository

    @Inject
    lateinit var mainRepository: MainRepository

    private var hashMapBleDevicesSortedByTime: HashMap<String, MutableList<BLEDevice>?> = HashMap()

    // LIFECYCLE FUNCTIONS
    override fun onCreate() {
        super.onCreate()

        observeTrackingPreferences()
        createBleDeviceHashMapWithMacAsKeyOrderedDescByTime()

        ortEnvironment = OrtEnvironment.getEnvironment()

        val modelPath = "Classifier.ort"
        val assetManager = assets
        val modelInputStream = assetManager.open(modelPath)
        val modelBytes = modelInputStream.readBytes()

        ortSession = ortEnvironment.createSession(modelBytes)


    }


    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {

        intent?.let {

            when (it.action) {

                ACTION_START_TRACKER_CLASSIFICATION_SERVICE -> {
                    if (!isServiceRunning) {
                        Timber.i("Start Service")
                        isServiceRunning = true
                        startForegroundService()
                    }
                }

                ACTION_STOP_TRACKER_CLASSIFICATION_SERVICE -> {
                    Timber.i("Stop Service")
                    isServiceRunning = false
                    stopSelf()
                }
            }
        }

        return super.onStartCommand(intent, flags, startId)
    }


    // FUNCTIONS USED IN LIFECYCLE (for code readability)
    private fun observeTrackingPreferences() {
        trackingPreferencesRepository.distance.asLiveData().observe(this) { distance = it }
        trackingPreferencesRepository.timeInMin.asLiveData().observe(this) { timeinMin = it }
        trackingPreferencesRepository.occurrences.asLiveData().observe(this) {
            occurrences = it
        }
        trackingPreferencesRepository.isRssiShieldEnabled.asLiveData().observe(this){isRssiShield = it}
        trackingPreferencesRepository.isLos.asLiveData().observe(this){isLos = it}
        trackingPreferencesRepository.isIndoor.asLiveData().observe(this){isIndoor = it}
    }

    private fun createBleDeviceHashMapWithMacAsKeyOrderedDescByTime() {

        mainRepository.getAllBLEDevicesSortedByTimestamp().observe(this) { bleDevices ->

            deleteBLEDevicesOlderThanTwoHours()

            hashMapBleDevicesSortedByTime.clear()

            for (bleDevice in bleDevices) {
                if (hashMapBleDevicesSortedByTime.containsKey(bleDevice.macAddress)) {

                    hashMapBleDevicesSortedByTime[bleDevice.macAddress]!!.add(bleDevice)

                } else {
                    hashMapBleDevicesSortedByTime[bleDevice.macAddress!!]=
                        mutableListOf(bleDevice)
                }
            }
        }
    }


    private fun startForegroundService() {

        startTrackerClassification()

        val notificationManager = getSystemService(Context.NOTIFICATION_SERVICE)
                as NotificationManager

        createNotificationChannel(notificationManager)

        val notificationBuilder = NotificationCompat.Builder(this,
            CHANNEL_ID_TRACKER_CLASSIFICATION
        )
            .setAutoCancel(false)
            .setOngoing(true)
            .setSmallIcon(R.drawable.ic_protect_48px)
            .setContentTitle("Home Scout")
            .setContentText("Tracker classification is running.")
            .setContentIntent(getMainActivityPendingIntent())

        startForeground(NOTIFICATION_ID_TRACKER_CLASSIFICATION, notificationBuilder.build())
    }

    private fun createNotificationChannel(notificationManager: NotificationManager) {
        val channel = NotificationChannel(
            CHANNEL_ID_TRACKER_CLASSIFICATION,
            NOTIFICATION_CHANNEL_TRACKER_CLASSIFICATION,
            IMPORTANCE_LOW
        )
        notificationManager.createNotificationChannel(channel)
    }

    private fun getMainActivityPendingIntent() = PendingIntent.getActivity(
        this,
        0,
        Intent(this, MainActivity::class.java).also {
            it.action = ACTION_SHOW_SETTINGS_FRAGMENT
        },
        FLAG_IMMUTABLE
    )

    private fun startTrackerClassification() {

        if (isServiceRunning) {

            hashMapBleDevicesSortedByTime.let{ hashMapBleDevicesSortedByTime ->

                for (key in hashMapBleDevicesSortedByTime.keys) {

                    // get all scans of the same mac address
                    val scansOfThisDevice = hashMapBleDevicesSortedByTime[key]!!

                    // check if more than one scan exists or it has less scans than defined by user
                    if ( scansOfThisDevice.size == 1 || scansOfThisDevice.size < occurrences!!) {continue}


                    // check if the tracker follows according to time defined by user
                    val youngestScanTime = scansOfThisDevice.first().timestampInMilliSeconds
                    val oldestScanTime = scansOfThisDevice.last().timestampInMilliSeconds
                    val diffBetweenYoungestAndOldestScan = youngestScanTime - oldestScanTime
                    val timeThresholdInMillis = timeinMin!! * 60000
                    if (diffBetweenYoungestAndOldestScan < timeThresholdInMillis) { continue }


                    // check if the tracker follows according to distance defined by user
                    var distanceFollowed = 0.0
                    val secondLastIndex = scansOfThisDevice.size - 2
                    for (i in 0..secondLastIndex) {

                        val currentLocation = Location("currentLocation").apply {
                            latitude = scansOfThisDevice[i].lat
                            longitude = scansOfThisDevice[i].lng
                        }

                        val nextLocation = Location("nextLocation").apply {
                            latitude = scansOfThisDevice[i + 1].lat
                            longitude = scansOfThisDevice[i + 1].lng
                        }

                        val distanceBetweenTwoLocations = currentLocation.distanceTo(nextLocation)
                        distanceFollowed += distanceBetweenTwoLocations
                    }

                    if (distanceFollowed < distance!!) { continue }

                    //Timber.i("RSSI shield about to be reached.")


                    //ML RSSI-Shielding
                    if (scansOfThisDevice[0].type == AirTag().type && isRssiShield == true) {
                        //Timber.i("Starting RSSI-Shielding for device of type AirTag. Number of scans: ${scansOfThisDevice.size}")
                        //Timber.i("Starting RSSI-Shielding for device of type AirTag. MAC Adress: ${scansOfThisDevice.first().macAddress}")

                        var closeTrackerCount = 0

                        for ((index, scan) in scansOfThisDevice.withIndex()) {
                            try {
                                // Log input preparation
                                val inputData = floatArrayOf(
                                    scan.RSSI.toFloat(),
                                    if (isIndoor == true) 1f else 0f,
                                    if (isLos == true) 1f else 0f
                                )
                                /**
                                Timber.i(
                                    "Processing scan $index for RSSI-Shielding. " +
                                        "Input Data: RSSI=${inputData[0]}, isIndoor=${inputData[1]}, isLos=${inputData[2]}"
                                )
                                **/
                                // Create input tensor and run inference
                                val inputTensor = OnnxTensor.createTensor(ortEnvironment, arrayOf(inputData))
                                val results = ortSession.run(Collections.singletonMap("input", inputTensor))
                                val outputTensor = results[0].value as LongArray
                                val outputValue = results[0].value

                                // Extract prediction and log the result
                                val prediction = outputTensor[0]
                                //Timber.i("Prediction result for scan $index: $prediction")
                                // Increment close tracker count if prediction matches
                                if (prediction == 1L) {
                                    closeTrackerCount++
                                    //Timber.i("Scan $index classified as 'close tracker'. Current close tracker count: $closeTrackerCount")
                                }

                                // Cleanup resources
                                inputTensor.close()
                                results.close()

                            } catch (e: Exception) {
                                // Log any errors encountered during inference
                                Timber.e("Error during RSSI-Shielding inference for scan $index: ${e.message}")
                                Timber.i(e.stackTraceToString())
                            }
                        }

                        //Timber.i("RSSI-Shielding completed. Close tracker count: $closeTrackerCount (Threshold: $occurrences)")
                        if (closeTrackerCount < occurrences!!) {
                            //Timber.i("Device does not meet the close tracker threshold. Skipping.")
                            continue
                        } else {
                            //Timber.i("Device passed the RSSI-Shielding criteria. Malicious tracker found.")
                        }
                    }
                    // FOUND A MALICIOUS TRACKER ACCORDING TO USER DEFINED PARAMETERS
                    val tracker = scansOfThisDevice.first()

                    val maliciousTracker = MaliciousTracker(
                        mac = tracker.macAddress!!,
                        timestampInMilliSeconds = tracker.timestampInMilliSeconds,
                        type = tracker.type)

                    insertMaliciousTrackerIfNotExistsAndNotify(maliciousTracker)
                }
            }

            handler.postDelayed({
                startTrackerClassification()
            }, INTERVAL_TRACKER_CLASSIFICATION)
        }
    }

    private fun sendFoundTrackerNotification() {
        val notificationManager = getSystemService(Context.NOTIFICATION_SERVICE)
                as NotificationManager

        createFoundTrackerNotificationChannel(notificationManager)

        val notificationBuilder = NotificationCompat.Builder(this,
            Constants.CHANNEL_ID_FOUND_DEVICE
        )
            .setAutoCancel(true)
            .setSmallIcon(R.drawable.ic_notifications_24px)
            .setContentTitle("Home Scout")
            .setContentText("Found a BLE Tracker")
            .setContentIntent(getMainActivityPendingIntentFoundTracker())

        notificationManager.notify(Constants.NOTIFICATION_ID_FOUND_DEVICE,notificationBuilder.build() )

    }

    private fun createFoundTrackerNotificationChannel(notificationManager: NotificationManager) {

        val channel = NotificationChannel(
            Constants.CHANNEL_ID_FOUND_DEVICE,
            Constants.NOTIFICATION_CHANNEL_FOUND_DEVICE,
            NotificationManager.IMPORTANCE_HIGH
        )
        notificationManager.createNotificationChannel(channel)

    }

    private fun getMainActivityPendingIntentFoundTracker() = PendingIntent.getActivity(
        this,
        0,
        Intent(this, MainActivity::class.java).also {
            it.action = ACTION_SHOW_NOTIFICATIONS_FRAGMENT
        },
        FLAG_IMMUTABLE
    )

    private fun deleteBLEDevicesOlderThanTwoHours() {
        lifecycleScope.launch{
            mainRepository.deleteBLEDevicesOlderThanTwoHours()
        }
    }

    private fun insertMaliciousTrackerIfNotExistsAndNotify(maliciousTracker: MaliciousTracker) {
        lifecycleScope.launch {
            val checkTracker = mainRepository.getMaliciousTrackerByMac(maliciousTracker.mac)
            if (checkTracker == null) {
                mainRepository.insertMaliciousTracker(maliciousTracker)
                sendFoundTrackerNotification()
            }
        }
    }
}
