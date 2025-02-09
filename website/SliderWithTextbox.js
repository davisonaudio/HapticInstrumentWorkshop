class SliderWithTextbox {
  constructor(lowerVal, upperVal, defaultVal, stepVal, sendValFunction) {
    this.lowerVal = lowerVal;
    this.upperVal = upperVal;
    this.sendValFunction = sendValFunction;
    
    this.slider = createSlider(lowerVal,upperVal,defaultVal,stepVal);
    this.textBox = createInput('');
    this.submitButton = createButton('Update');
    this.textBox.size(50);
    
    this.slider.input(this.sliderChanged);
    this.submitButton.mousePressed(this.updatePressed);
    this.textBox.value(this.slider.value());
  }
  
  position(x, y){
    this.textBox.position(x, y);
    this.submitButton.position(this.textBox.x + this.textBox.width + 10, this.textBox.y);
    this.slider.position(x, y + this.textBox.height + 10);
  }
  
  updatePressed(){
    let sendVal = constrain(this.textBox.value(), this.lowerVal, this.upperVal);
    this.slider.value(sendVal);
    this.sendValFunction(sendVal);
  }
  
  sliderChanged(){
    this.textBox.value(this.slider.value());
    this.sendValFunction(this.slider.value());
  }
}