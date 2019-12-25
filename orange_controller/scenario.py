# -*- coding: utf-8 -*-
from quest_module import *

def isr_callback():
	pass

def GPIO_IN(id):
	return GPIO(id, GPIO.Type.IN, isr_callback)

def QUEST_SOUND(path):
	return '/media/flash/ДСП/{}.mp3'.format(path)

def QUEST_SECONDS(x):
	return (x * 1000)

def QUEST_MINUTES(x):
	return (x * 60000)

backgroundPlayer = Mpg123Player()
oneShotPlayer = Mpg123Player()

class LedPinHelper():
	def __init__(self, pin):
		self._pin = GPIO(pin)

	def setValue(self, val):
		if self._pin.getValue() != val :
			self._pin.setValue(val)

class DomushButton():
	def __init__(self):
		self._domushTest = GPIO_IN(414)
		self._domushState = self._domushTest.getValue()
		self._ledPin = LedPinHelper(415)

	def isPressed(self):
		newState = self._domushTest.getValue()
		pressed = False
		if newState == GPIO.Value.LOW and newState != self._domushState:
			pressed = True

		self._ledPin.setValue(self._domushState)

		self._domushState = newState
		return pressed

domushButton = DomushButton()

class PuzzleIntro(PuzzleBase):
	def __init__(self):
		PuzzleBase.__init__(self)
		self.setInstanceName("Интро")
		self.setDuration(QUEST_MINUTES(5))

	def onRun(self):
		backgroundPlayer.play(QUEST_SOUND("ФОН  (дом с привидениями)"), 120)

	def onStop(self):
		backgroundPlayer.stop()

	def onSolve(self):
		oneShotPlayer.play(QUEST_SOUND("открытие тайника (волшебная №1)"))

class PuzzleHanger(PuzzleBase):
	def __init__(self):
		PuzzleBase.__init__(self)
		self.setInstanceName("Ящик под вешалкой")
		self._testPin = GPIO_IN(409)
		self._magnetPin = GPIO(410)
		self.setDuration(QUEST_MINUTES(10))

	def onArm(self):
		self._magnetPin.setValue(GPIO.Value.HIGH)

	def onTest(self):
		return self._testPin.getValue() == GPIO.Value.HIGH

	def onStop(self):
		self._magnetPin.setValue(GPIO.Value.LOW)

	def onSolve(self):
		oneShotPlayer.play(QUEST_SOUND("открытие тайника (волшебная №2)"))
		self.onStop()

class PuzzleClock(PuzzleBase):
	def __init__(self):
		PuzzleBase.__init__(self)
		self.setInstanceName("Ходики")
		self._testPin = GPIO_IN(412)
		self._magnetPin = GPIO(411)
		self.setDuration(QUEST_MINUTES(10))

	def onArm(self):
		self._magnetPin.setValue(GPIO.Value.HIGH)

	def onTest(self):
		return self._testPin.getValue() == GPIO.Value.LOW

	def onStop(self):
		self._magnetPin.setValue(GPIO.Value.LOW)

	def onSolve(self):
		oneShotPlayer.play(QUEST_SOUND("Тайник часы (тик-так)"))
		self.onStop()

class PuzzleWaitDesk(PuzzleBase):
	def __init__(self, dur):
		PuzzleBase.__init__(self)
		self.setInstanceName("Ожидание фразы на доске")
		self.setDuration(QUEST_SECONDS(dur))

	def onSolve(self):
		oneShotPlayer.play(QUEST_SOUND("открытие тайника (волшебная №2)"))

	def onTest(self):
		if not self.isRunning():
			return False

		mb, data = MODBUS.ReadRegisters(2, 0)
		return (mb == 1 and data == 0) or mb == -1 #if board do not respond to status poll - do not wait

class PuzzleDesk(PuzzleBase):
	def __init__(self, phrase):
		PuzzleBase.__init__(self)
		self.setInstanceName("Спиритическая доска. " + phrase)
		self.setDuration(QUEST_MINUTES(10))
		self._phrase = phrase

	def onSolve(self):
		oneShotPlayer.play(QUEST_SOUND("открытие тайника (волшебная №1)"))
		MODBUS.WriteRegisters(2, 0, self._phrase)

class PuzzleLibrary(PuzzleBase):
	def __init__(self):
		PuzzleBase.__init__(self)
		self.setInstanceName("Дверь-стеллаж")
		self._testPin = GPIO_IN(392)
		self._magnetPin = GPIO(393)
		self._ledPin = GPIO(394)
		self.setDuration(QUEST_MINUTES(10))

	def onRun(self):
		self._ledPin.setValue(GPIO.Value.HIGH)

	def onTest(self):
		return self._testPin.getValue() == GPIO.Value.LOW

	def onArm(self):
		self._magnetPin.setValue(GPIO.Value.HIGH)
		MODBUS.WriteRegister(1, 0, 1)  # close the door

	def onStop(self):
		self._magnetPin.setValue(GPIO.Value.LOW)
		self._ledPin.setValue(GPIO.Value.LOW)
		MODBUS.WriteRegister(1, 0, 0)  # open the door

	def onSolve(self):
		oneShotPlayer.play(QUEST_SOUND("открытие двери в библиотеку"), 200)
		self.onStop()


class PuzzleDomushFlipBook(PuzzleBase):
	def __init__(self):
		PuzzleBase.__init__(self)
		self.setInstanceName("Домуш Полка")
		self._magnetPin = GPIO(406)
		self.setDuration(QUEST_MINUTES(10))

	def onTest(self):
		if self.isRunning() and domushButton.isPressed() :
			oneShotPlayer.play(QUEST_SOUND("Бюст 1 подсказка (про книгу)"), 300)

	def onArm(self):
		self._magnetPin.setValue(GPIO.Value.HIGH)

	def onStop(self):
		self._magnetPin.setValue(GPIO.Value.LOW)

	def onSolve(self):
		oneShotPlayer.play(QUEST_SOUND("открытие тайника (волшебная №2)"))
		self.onStop()

class PuzzleDomushCandle(PuzzleBase):
	def __init__(self):
		PuzzleBase.__init__(self)
		self.setInstanceName("Домуш свечи")

		self._testPin = GPIO_IN(388)
		self._magnetPin = GPIO(405)
		self.setDuration(QUEST_MINUTES(10))

	def onArm(self):
		self._magnetPin.setValue(GPIO.Value.HIGH)

	def onTest(self):
		if self.isRunning() and domushButton.isPressed() :
			oneShotPlayer.play(QUEST_SOUND("Бюст 2 подсказка (про свечи)"), 300)

		return self._testPin.getValue() == GPIO.Value.LOW

	def onStop(self):
		self._magnetPin.setValue(GPIO.Value.LOW)

	def onSolve(self):
		oneShotPlayer.play(QUEST_SOUND("открытие тайника (волшебная №1)"))
		self.onStop()

class PuzzleDomushCanary(PuzzleBase):
	def __init__(self):
		PuzzleBase.__init__(self)
		self.setInstanceName("Домуш канарейка")
		self._testPin = GPIO_IN(403)
		self._magnetPin = GPIO(402)
		self.setDuration(QUEST_MINUTES(10))

	def onArm(self):
		self._magnetPin.setValue(GPIO.Value.HIGH)

	def onStop(self):
		self._magnetPin.setValue(GPIO.Value.LOW)

	def onTest(self):
		if self.isRunning() and domushButton.isPressed() :
			oneShotPlayer.play(QUEST_SOUND("Бюст 3 подсказка (про канарейку)"), 300)

		return self._testPin.getValue() == GPIO.Value.HIGH

	def onSolve(self):
		oneShotPlayer.play(QUEST_SOUND("открытие тайника (волшебная №2)"))
		self.onStop()

class PuzzleDomushArt(PuzzleBase):
	def __init__(self):
		PuzzleBase.__init__(self)
		self.setInstanceName("Домуш картины")
		self._testPin = GPIO_IN(404)
		self._magnetPin = GPIO(401)
		self.setDuration(QUEST_MINUTES(10))

	def onArm(self):
		self._magnetPin.setValue(GPIO.Value.HIGH)

	def onTest(self):

		if self.isRunning() and domushButton.isPressed() :
			oneShotPlayer.play(QUEST_SOUND("Бюст 4 подсказка (про глаза)"), 300)

		return self._testPin.getValue() == GPIO.Value.LOW

	def onStop(self):
		self._magnetPin.setValue(GPIO.Value.LOW)

	def onSolve(self):
		oneShotPlayer.play(QUEST_SOUND("открытие тайника (волшебная №1)"))
		self.onStop()

class PuzzleDomushAqua(PuzzleBase):
	def __init__(self):
		PuzzleBase.__init__(self)
		self.setInstanceName("Домуш Аквариум")

		self._ledPin = GPIO(413)
		self._talk = False
		self.setDuration(QUEST_MINUTES(10))

	def onTest(self):
		if self.isRunning() and domushButton.isPressed() :
			self._talk = True
			oneShotPlayer.play(QUEST_SOUND("Бюст 5 подсказка (аквариум)"), 300)

		return (self._talk == True and oneShotPlayer.isPlaying() == False ) # Trigger puzzle when tip talk is finished

	def onArm(self):
		self._talk = False
		self._ledPin.setValue(GPIO.Value.LOW)

	def onSolve(self):
		oneShotPlayer.play(QUEST_SOUND("открытие тайника (волшебная №2)"))
		self._ledPin.setValue(GPIO.Value.HIGH)	

	def onStop(self):
		self._ledPin.setValue(GPIO.Value.LOW)

class PuzzleDomushAmulet(PuzzleBase):
	def __init__(self):
		PuzzleBase.__init__(self)
		self.setInstanceName("Домуш амулет")

		self._magnetPin = GPIO(400)
		self.setDuration(QUEST_MINUTES(10))

	def onTest(self):
		if self.isRunning() and domushButton.isPressed() :
			oneShotPlayer.play(QUEST_SOUND("Бюст 5 подсказка (аквариум)"), 300)

	def onArm(self):
		self._magnetPin.setValue(GPIO.Value.HIGH)

	def onSolve(self):
		oneShotPlayer.play(QUEST_SOUND("открытие тайника (волшебная №2)"))
		self.onStop()	

	def onStop(self):
		self._magnetPin.setValue(GPIO.Value.LOW)

class PuzzleExit(PuzzleBase):
	def __init__(self):
		PuzzleBase.__init__(self)
		self.setInstanceName("Выход")

		self._magnetPin = GPIO(384)
		self.setDuration(QUEST_MINUTES(10))

	def onArm(self):
		self._magnetPin.setValue(GPIO.Value.HIGH)

	def onStop(self):
		self._magnetPin.setValue(GPIO.Value.LOW)

	def onSolve(self):
		oneShotPlayer.play(QUEST_SOUND("Библиотека ФИНАЛЬНАЯ (фанфары победы)"), 200)
		self.onStop()

class offPuzzle(PuzzleBase):
	def __init__(self):
		PuzzleBase.__init__(self)
		self.setInstanceName("OffPuzzle")
		self._relay = GPIO(15)
		self._relay220V = GPIO(389)

	def onArm(self):
		self._relay.setValue(GPIO.Value.LOW)
		self._relay220V.setValue(GPIO.Value.HIGH)

	def onSolve(self):
		self._relay.setValue(GPIO.Value.HIGH)
		self._relay220V.setValue(GPIO.Value.LOW)

class nextPuzzle(PuzzleBase):
	def __init__(self):
		PuzzleBase.__init__(self)
		self._nextKey = GPIO_IN(407)

	def onTest(self):
		return self._nextKey.getValue() == GPIO.Value.LOW

class powerButtonPuzzle(PuzzleBase):
	def __init__(self):
		PuzzleBase.__init__(self)
		self._powerKey = GPIO_IN(391)

	def onTest(self):
		return self._powerKey.getValue() == GPIO.Value.LOW

class serviceLightPuzzle(PuzzleBase):
	def __init__(self):
		PuzzleBase.__init__(self)
		self._lightGPIO = GPIO(390)

	def onArm(self):
		self._lightGPIO.setValue(GPIO.Value.HIGH)

	def onSolve(self):
		self._lightGPIO.setValue(GPIO.Value.LOW)

class serviceLightButtonPuzzle(PuzzleBase):
	def __init__(self):
		PuzzleBase.__init__(self)
		self._lightKey = GPIO_IN(408)

	def onTest(self):
		return self._lightKey.getValue() == GPIO.Value.LOW

MODBUS = Modbus("/dev/ttyS1")

set_quest_name("Дом с привидениями")

intro = PuzzleIntro()
add_puzzle(intro)

phanger = PuzzleHanger()
add_puzzle(phanger)

pclock = PuzzleClock()
add_puzzle(pclock)

pdesk1 = PuzzleDesk("привет вы пришли мне помочь")
add_puzzle(pdesk1)

#pdeskWait1 = PuzzleWaitDesk()
#add_puzzle(pdeskWait1)

pdesk2 = PuzzleDesk("какой у вас вопрос")
add_puzzle(pdesk2)
#pdeskWait2 = PuzzleWaitDesk()
#add_puzzle(pdeskWait2)

pdesk3 = PuzzleDesk("вам поможет радуга")
add_puzzle(pdesk3)
pdeskWait3 = PuzzleWaitDesk(50)
add_puzzle(pdeskWait3)

pdoor = PuzzleLibrary()
add_puzzle(pdoor)

dflip = PuzzleDomushFlipBook()
add_puzzle(dflip)

dcandle = PuzzleDomushCandle()
add_puzzle(dcandle)

dcanary = PuzzleDomushCanary()
add_puzzle(dcanary)

dart = PuzzleDomushArt()
add_puzzle(dart)

daqua = PuzzleDomushAqua()
add_puzzle(daqua)

damulet = PuzzleDomushAmulet()
add_puzzle(damulet)

pe = PuzzleExit()
add_puzzle(pe)

off = offPuzzle()
nextp = nextPuzzle()
powerb = powerButtonPuzzle()
sl = serviceLightPuzzle()
slb = serviceLightButtonPuzzle()
set_special_puzzles(off, nextp, powerb, sl, slb)
