package android.example.homescout.ui.settings

import android.example.homescout.repositories.TrackingPreferencesRepository
import androidx.lifecycle.*
import dagger.hilt.android.lifecycle.HiltViewModel
import kotlinx.coroutines.launch
import javax.inject.Inject

@HiltViewModel
class SettingsViewModel @Inject constructor(
    private val trackingPreferencesRepository : TrackingPreferencesRepository
): ViewModel() {

    private val _isSwitchEnabled = MutableLiveData<Boolean>()
    val isSwitchEnabled: LiveData<Boolean>
        get() = trackingPreferencesRepository.isTrackingEnabled.asLiveData()

    private val _distance = MutableLiveData<Float>()
    val distance: LiveData<Float>
        get() = trackingPreferencesRepository.distance.asLiveData()

    private val _timeInMin = MutableLiveData<Float>()
    val timeInMin: LiveData<Float>
        get() = trackingPreferencesRepository.timeInMin.asLiveData()

    private val _occurrences = MutableLiveData<Float>()
    val occurrences: LiveData<Float>
        get() = trackingPreferencesRepository.occurrences.asLiveData()

    val isRssiShield: LiveData<Boolean>
        get() = trackingPreferencesRepository.isRssiShieldEnabled.asLiveData()

    val isLos: LiveData<Boolean>
        get() = trackingPreferencesRepository.isLos.asLiveData()

    val isIndoor: LiveData<Boolean>
        get() = trackingPreferencesRepository.isIndoor.asLiveData()

    fun updateIsRssiShield(value: Boolean) {
        viewModelScope.launch {
            trackingPreferencesRepository.updateIsRssiShield(value)
        }
    }

    fun updateIsLos(value: Boolean) {
        viewModelScope.launch {
            trackingPreferencesRepository.updateIsLos(value)
        }
    }

    fun updateIsIndoor(value: Boolean) {
        viewModelScope.launch {
            trackingPreferencesRepository.updateIsIndoor(value)
        }
    }

    fun onSwitchToggled(checked: Boolean) {
        viewModelScope.launch {
            trackingPreferencesRepository.updateIsTrackingEnabled(checked)
        }
    }

    fun updateDistance(value: Float) {
        viewModelScope.launch {
            trackingPreferencesRepository.updateDistance(value)
        }
    }

    fun updateTimeInMin(value: Float) {
        viewModelScope.launch {
            trackingPreferencesRepository.updateTimeInMin(value)
        }
    }

    fun updateOccurrences(value: Float) {
        viewModelScope.launch {
            trackingPreferencesRepository.updateOccurrences(value)
        }
    }

}
