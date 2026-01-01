"""
Solar IQ - Vision Transformer Inference
Classifies solar panel conditions from images
"""

import torch
from transformers import ViTForImageClassification, ViTFeatureExtractor
from PIL import Image
import requests

class SolarPanelClassifier:
    def __init__(self, model_path='./best_model.pth'):
        self.device = torch.device('cuda' if torch.cuda.is_available() else 'cpu')
        self.model = ViTForImageClassification.from_pretrained(
            'google/vit-base-patch16-224',
            num_labels=4
        )
        self.model.load_state_dict(torch.load(model_path, map_location=self.device))
        self.model.eval()
        
        self.feature_extractor = ViTFeatureExtractor.from_pretrained(
            'google/vit-base-patch16-224'
        )
        
        self.classes = ['normal', 'dust', 'shading', 'bird_droppings']
    
    def predict(self, image_path):
        """Predict condition from image"""
        image = Image.open(image_path).convert('RGB')
        inputs = self.feature_extractor(images=image, return_tensors="pt")
        inputs = {k: v.to(self.device) for k, v in inputs.items()}
        
        with torch.no_grad():
            outputs = self.model(**inputs)
            logits = outputs.logits
            probs = torch.nn.functional.softmax(logits, dim=-1)
            
        confidence, predicted = torch.max(probs, 1)
        
        return {
            'condition': self.classes[predicted.item()],
            'confidence': confidence.item(),
            'all_probabilities': {
                cls: prob.item() 
                for cls, prob in zip(self.classes, probs[0])
            }
        }

# Usage
if __name__ == '__main__':
    classifier = SolarPanelClassifier()
    result = classifier.predict('panel_image.jpg')
    print(f"Condition: {result['condition']}")
    print(f"Confidence: {result['confidence']:.2%}")
